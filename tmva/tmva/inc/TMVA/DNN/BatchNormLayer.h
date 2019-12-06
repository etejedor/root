
// Author: Vladimir Ilievski

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : TBatchNormLayer                                                           *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Dense Layer Class                                                         *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Vladimir Ilievski      <ilievski.vladimir@live.com>  - CERN, Switzerland  *
 *                                                                                *
 * Copyright (c) 2005-2015:                                                       *
 *      CERN, Switzerland                                                         *
 *      U. of Victoria, Canada                                                    *
 *      MPI-K Heidelberg, Germany                                                 *
 *      U. of Bonn, Germany                                                       *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

#ifndef TMVA_DNN_BatchNormLayer
#define TMVA_DNN_BatchNormLayer

//#include "TMatrix.h"

#include "TMVA/DNN/GeneralLayer.h"
#include "TMVA/DNN/Functions.h"

#include "TMVA/DNN/Architectures/Reference.h"

#include "TMVA/DNN/CNN/ContextHandles.h"

#include <iostream>
#include <iomanip>

namespace TMVA {
namespace DNN {

/** \class TBatchNormLayer

      Layer implementing Batch Normalization

     The input from each batch are normalized during training to have zero mean and unit variance
     and they are then scaled by two parameter, different for each input variable:
      - a scale factor gamma
      - an offset beta

   In addition a running batch mean and variance is computed and stored in the class
   During inference the inputs are not normalized using the batch mean but the previously computed
  at  running mean and variance
   If momentum is in [0,1) the running mean and variances are the exponetial averages using the momentum value
     runnig_mean = momentum * running_mean + (1-momentum) * batch_mean
   If instead momentum<1 the cumulative average is computed
   running_mean = (nb/(nb+1) * running_mean + 1/(nb+1) * batch_mean

   See more at [https://arxiv.org/pdf/1502.03167v3.pdf]
*/
template <typename Architecture_t>
class TBatchNormLayer : public VGeneralLayer<Architecture_t> {
public:

   using Scalar_t = typename Architecture_t::Scalar_t;
   using Matrix_t = typename Architecture_t::Matrix_t;
   using Tensor_t = typename Architecture_t::Tensor_t;

   using HelperDescriptor_t  = typename Architecture_t::TensorDescriptor_t;
   using BNormDescriptors_t = typename Architecture_t::BNormDescriptors_t;

   //using Matrix_t = TMatrixD;

private:

   Tensor_t fDerivatives; ///< First fDerivatives of the activations of this layer.

   int      fNormAxis; ///< Normalization axis. For each element of this axis we will compute mean and stddev

   Scalar_t fMomentum; ///< The weight decay.
   Scalar_t fEpsilon;

   //Matrix_t fXmu;
   //Matrix_t fXhat;

   // Matrix_t dgammax;
   //Matrix_t fVar;
   // Matrix_t sqrtvar;
   //Matrix_t fIvar;

   Matrix_t fMu;
   Matrix_t fVar;
   Matrix_t fIVar;

   // Matrix_t fGamma;
   // Matrix_t fBeta;
   Matrix_t fMu_Training;
   Matrix_t fVar_Training;

   // cached tensor used for Cudnn to get correct shape
   Tensor_t fReshapedData;  // cached reshaped data tensor

   // counter of trained batches for computing tesing and variance means
   int fTrainedBatches = 0;

   TDescriptors * fDescriptors = nullptr;

public:
   /*! Constructor */
   TBatchNormLayer(size_t batchSize, size_t inputDepth, size_t inputHeight, size_t inputWidth,
                   const std::vector<size_t> & shape, int axis = -1, Scalar_t momentum = -1., Scalar_t epsilon = 0.0001);

   /*! Copy the dense layer provided as a pointer */
   TBatchNormLayer(TBatchNormLayer<Architecture_t> *layer);

   /*! Copy Constructor */
   TBatchNormLayer(const TBatchNormLayer &);

   /*! Destructor */
   ~TBatchNormLayer();

   /*! Compute activation of the layer for the given input. The input
    * must be in 3D tensor form with the different matrices corresponding to
    * different events in the batch. Computes activations as well as
    * the first partial derivative of the activation function at those
    * activations. */
   void Forward(Tensor_t &input, bool inTraining = true);

   /*! Compute weight, bias and activation gradients. Uses the precomputed
    *  first partial derviatives of the activation function computed during
    *  forward propagation and modifies them. Must only be called directly
    *  a the corresponding call to Forward(...). */
   void Backward(Tensor_t &gradients_backward, const Tensor_t &activations_backward);
   //              Tensor_t &inp1, Tensor_t &inp2);


   /* reset at end of training the batch counter */
   void ResetTraining() { fTrainedBatches = 0; }

   /*! Printing the layer info. */
   void Print() const;

   /*! Writes the information and the weights about the layer in an XML node. */
   virtual void AddWeightsXMLTo(void *parent);

   /*! Read the information and the weights about the layer from XML node. */
   virtual void ReadWeightsFromXML(void *parent);

   /* initialize weights */
   virtual void Initialize();

   /*  get number of trained batches */
   const int & GetNTrainedBatches() const { return fTrainedBatches;}
   int & GetNTrainedBatches() { return fTrainedBatches;}

   /*  get batch means for the training phase */
   const Matrix_t & GetBatchMean() const { return fMu;}
   Matrix_t & GetBatchMean() { return fMu;}

   /*  Get the normalized batch examples */
   //const Matrix_t & GetNormedBatch() const { return fXhat;}
   //Matrix_t & GetNormedBatch() { return fXhat;}

   /*  Get the gradient of gamma for backpropagation */
   const Matrix_t & GetVariance() const { return fVar;}
   Matrix_t & GetVariance() { return fVar;}

   /*  Get the sqrt of the batch variances for the training phase */
   const Matrix_t & GetIVariance() const { return fIVar;}
   Matrix_t & GetIVariance() { return fIVar;}

   /*  get vector of averages computed in the training phase */
   const Matrix_t & GetMuVector() const { return fMu_Training;}
   Matrix_t & GetMuVector() { return fMu_Training;}

   /*  get vector of variances computed in the training phase */
   const Matrix_t & GetVarVector() const { return fVar_Training;}
   Matrix_t & GetVarVector()  { return fVar_Training;}

   // Scalar_t GetWeightDecay() const { return fWeightDecay; }

   /*  Get the momentum of the running mean/variance */
   Scalar_t GetMomentum() const { return fMomentum;}

   /*  Get epsilon */
   Scalar_t GetEpsilon() const { return fEpsilon;}

   /*  Get normalization axis (the one which will have each element normalized) */
   Scalar_t GetNormAxis() const { return fNormAxis;}

   const Matrix_t &GetReshapedData() const { return fReshapedData; }
   Matrix_t &GetReshapedData() { return fReshapedData; }

protected:
   static size_t CalculateNormDim(int axis, size_t c, size_t h, size_t w)
   {
      if (axis == -1)
         return c * h * w;
      else if (axis == 1)
         return c;
      else if (axis == 2)
         return h;
      else if (axis == 3)
         return w;
      return 0;
      }
};


//
//
//  The Dense Layer Class - Implementation
//______________________________________________________________________________
template <typename Architecture_t>
TBatchNormLayer<Architecture_t>::TBatchNormLayer(size_t batchSize, size_t inputDepth, size_t inputHeight,
                                                 size_t inputWidth, const std::vector<size_t> &shape, int axis,
                                                 Scalar_t momentum, Scalar_t epsilon)
   : VGeneralLayer<Architecture_t>(batchSize, inputDepth, inputHeight, inputWidth, // bs + input shape
                                   inputDepth, inputHeight, inputWidth,            // output shape
                                   2, 1,
                                   CalculateNormDim(axis, inputDepth, inputHeight, inputWidth), // weight tensor dim.
                                   1, 1, 1,                                                      // bias
                                   shape[2], shape[0], shape[1],                                 // output tensor shape as bsize, depth, hw
                                   EInitialization::kZero),
     fNormAxis(axis), fMomentum(momentum), fEpsilon(epsilon),
     fMu(1, VGeneralLayer<Architecture_t>::GetWeightsAt(0).GetNcols()), // dimension is same as weights
     fVar(1, VGeneralLayer<Architecture_t>::GetWeightsAt(0).GetNcols()),
     fIVar(1, VGeneralLayer<Architecture_t>::GetWeightsAt(0).GetNcols()),
     fMu_Training(1, VGeneralLayer<Architecture_t>::GetWeightsAt(0).GetNcols()),
     fVar_Training(1, VGeneralLayer<Architecture_t>::GetWeightsAt(0).GetNcols()),
     fReshapedData(1,1,1)  // use a dummy single element tensor

//   fXmu(batchSize, inputWidth), fXhat(batchSize, inputWidth),
//   // dgammax(batchSize, inputWidth),
//   fVar(1, inputWidth),
//   // sqrtvar(1, inputWidth),
//   fMu(calculateBNormDim(axis, inputDepth, inpuyHeight, inputWidth)),
//   fIvar(1, inputWidth), fMu_Training(inputWidth),
//   fVar_Training(inputWidth)
{

}
//______________________________________________________________________________
template <typename Architecture_t>
TBatchNormLayer<Architecture_t>::TBatchNormLayer(TBatchNormLayer<Architecture_t> *layer)
   : VGeneralLayer<Architecture_t>(layer)
{
   // to be implemented
   printf("Error - copy ctor not implmented\n");
}

//______________________________________________________________________________
template <typename Architecture_t>
TBatchNormLayer<Architecture_t>::TBatchNormLayer(const TBatchNormLayer &layer) : VGeneralLayer<Architecture_t>(layer)
{
   // to be implmeented
   printf("Error - copy ctor not implmented\n");
}

//______________________________________________________________________________
template <typename Architecture_t>
TBatchNormLayer<Architecture_t>::~TBatchNormLayer()
{
   // release descriptors
   if (fDescriptors) {
      Architecture_t::ReleaseBNormDescriptors(fDescriptors);
      delete fDescriptors;
   }
}

template <typename Architecture_t>
auto TBatchNormLayer<Architecture_t>::Initialize() -> void
{
   Matrix_t &gamma = this->GetWeightsAt(0);
   Matrix_t &beta = this->GetWeightsAt(1);
   size_t bndim = gamma.GetNcols(); // fMu_Training.size();
   // initialize<Architecture_t>(gamma, EInitialization::kIdentity);
   initialize<Architecture_t>(beta, EInitialization::kZero);
   for (size_t i = 0; i < bndim; ++i) {
      gamma(0, i) = 1.;
      // assign default values for the other parameters
      fMu_Training(0,i) = 0;
      fVar_Training(0,i) = 1;
   }

   Matrix_t &dgamma = this->GetWeightGradientsAt(0);
   Matrix_t &dbeta = this->GetWeightGradientsAt(1);
   initialize<Architecture_t>(dgamma, EInitialization::kZero);
   initialize<Architecture_t>(dbeta, EInitialization::kZero);

   fTrainedBatches = 0;

   Architecture_t::InitializeBNormDescriptors(fDescriptors, this);
}

//______________________________________________________________________________
template <typename Architecture_t>
auto TBatchNormLayer<Architecture_t>::Forward(Tensor_t &x, bool inTraining) -> void
{
   Tensor_t x2;
   Tensor_t y2;
   if (x.GetLayout() != fReshapedData.GetLayout()) {
      x2 = Tensor_t(x.GetDeviceBuffer(), fReshapedData.GetShape(), fReshapedData.GetLayout());
      y2 = Tensor_t(this->GetOutput().GetDeviceBuffer(), fReshapedData.GetShape(), fReshapedData.GetLayout());
   }
   else{
      x2 = x;
      y2 = this->GetOutput();
   }

   auto descr = static_cast<BNormDescriptors_t *> (fDescriptors);
   if (inTraining) {
      Architecture_t::BatchNormLayerForwardTraining(fNormAxis, x2, y2,
                                                    this->GetWeightsAt(0), this->GetWeightsAt(1),
                                                    this->GetBatchMean(), this->GetVariance(), this->GetIVariance(),
                                                    this->GetMuVector(),
                                                    this->GetVarVector(), this->GetNTrainedBatches(),
                                                    this->GetMomentum(), this->GetEpsilon(),
                                                    descr->HelperDescriptor);
      fTrainedBatches++;
   }

   else {
      Architecture_t::BatchNormLayerForwardInference(fNormAxis, x2, this->GetWeightsAt(0), this->GetWeightsAt(1),
                                                     y2, this->GetMuVector(), this->GetVarVector(),
                                                     this->GetEpsilon(), descr->HelperDescriptor);
      fTrainedBatches = 0;
   }
#if 0
   Matrix_t input = x.At(0).GetMatrix();

   Matrix_t &gamma = this->GetWeightsAt(0);
   Matrix_t &beta = this->GetWeightsAt(1);

   // gamma.Print();
   // beta.Print();

   Matrix_t out = this->GetOutputAt(0);
   double epsilon = fEpsilon;

   int n = input.GetNrows();
   int d = input.GetNcols();

   for (int k = 0; k < d; ++k) {

      if (inTraining) {

         double mean = 0;
         for (int i = 0; i < n; i++) {
            mean = mean + input(i, k);
         }
         mean = mean / n;

         for (int i = 0; i < n; i++) {
            fXmu(i, k) = input(i, k) - mean;
         }
         double sq = 0;
         for (int i = 0; i < n; i++) {
            sq = sq + (fXmu(i, k) * fXmu(i, k));
         }
         fVar(0, k) = sq / n;
         // fVar(0,k) = fVar(0,k) + epsilon;
         // sqrtvar(0,k) =
         fIvar(0, k) = 1. / std::sqrt(fVar(0, k) + epsilon);
         for (int i = 0; i < n; i++) {
            fXhat(i, k) = fXmu(i, k) * fIvar(0, k);
            out(i, k) = gamma(0, k) * fXhat(i, k) + beta(0, k);
         }

         // fVar(0,k) -= epsilon;

         if (fTrainedBatches == 0) {
            fMu_Training[k] = mean;
            fVar_Training[k] = fVar(0, k) * (n) / (Scalar_t(n - 1) + epsilon);
         } else {
            Scalar_t decay = fMomentum;
            if (fMomentum < 0) decay = fTrainedBatches/Scalar_t(fTrainedBatches+1);
            fMu_Training[k] = decay * fMu_Training[k] + (1. - decay) * mean;
            fVar_Training[k] = decay * fVar_Training[k] + (1.-decay) * fVar(0, k) * (n) / (Scalar_t(n - 1) + epsilon);
         }

      }
      // during inference just use stored mu and variance
      else {
         for (int i = 0; i < n; i++) {
            out(i, k) =
               gamma(0, k) * ((input(i, k) - fMu_Training[k]) / (sqrt(fVar_Training[k] + epsilon))) + beta(0, k);
         }
      }
   } // end loop on k
   if (inTraining) fTrainedBatches++;
   else fTrainedBatches = 0;
   // fVar.Print();
   // if (inTraining)
   //    std::cout << " training batch " << fTrainedBatches << " mu var0" << fMu_Training[0] << std::endl;
   // else
   //    std::cout << " testing batch  " << fTrainedBatches << " mu var0" << fMu_Training[0] << std::endl;
   # endif
}

//______________________________________________________________________________
template <typename Architecture_t>
auto TBatchNormLayer<Architecture_t>::Backward(Tensor_t &gradients_backward,
                                               const Tensor_t & activations_backward ) -> void
//                                               Tensor_t &, Tensor_t &) -> void
{
   auto descr = static_cast<BNormDescriptors_t *> (fDescriptors);


   if (activations_backward.GetLayout() != fReshapedData.GetLayout()) {
      Tensor_t x = Tensor_t(activations_backward.GetDeviceBuffer(), fReshapedData.GetShape(), fReshapedData.GetLayout());
      Tensor_t dx = Tensor_t(gradients_backward.GetDeviceBuffer(), fReshapedData.GetShape(), fReshapedData.GetLayout());
      Tensor_t dy = Tensor_t(this->GetActivationGradients().GetDeviceBuffer(), fReshapedData.GetShape(), fReshapedData.GetLayout());

      Architecture_t::BatchNormLayerBackward(fNormAxis, x, dy, dx,
                                             this->GetWeightsAt(0),           // gamma (beta is not needed)
                                             this->GetWeightGradientsAt(0), this->GetWeightGradientsAt(1),
                                             this->GetBatchMean(), this->GetVariance(), this->GetIVariance(),
                                             this->GetEpsilon(), descr->HelperDescriptor);

   } else {

      Architecture_t::BatchNormLayerBackward(fNormAxis, activations_backward, // x
                                          this->GetActivationGradients(), // dy
                                          gradients_backward,             // dx
                                          this->GetWeightsAt(0),          // gamma (beta is not needed)
                                          this->GetWeightGradientsAt(0), this->GetWeightGradientsAt(1),
                                          this->GetBatchMean(), this->GetVariance(), this->GetIVariance(),
                                          this->GetEpsilon(), descr->HelperDescriptor);
   }
# if 0
   double epsilon = fEpsilon;


   // inputs
   const Matrix_t &dout = this->GetActivationGradients().At(0).GetMatrix();
   const Matrix_t &gamma = this->GetWeightsAt(0);
   //const Matrix_t &x = activations_backward[0];
   int d = dout.GetNcols();
   int n = dout.GetNrows();

   // outputs gradients
   Matrix_t &dgamma = this->GetWeightGradientsAt(0);
   Matrix_t &dbeta = this->GetWeightGradientsAt(1);
   Matrix_t dx = gradients_backward.At(0).GetMatrix();

   // compute first gradients for gamma and beta
   for (int k = 0; k < d; k++) {
      dgamma(0, k) = 0;
      dbeta(0, k) = 0;
      for (int i = 0; i < n; i++) {
         dbeta(0, k) += dout(i, k);
         dgamma(0, k) += dout(i, k) * fXhat(i, k);
         // dxhat(i,k) = dout(i,k) * gamma(0,k);
      }
   }

   // compute gradients with respect to input
   double npSumDy = 0;
   double npSumDyHMu = 0;

   for (int k = 0; k < d; k++) {
      for (int i = 0; i < n; i++) {
         npSumDy += dout(i, k);
         npSumDyHMu += dout(i, k) * fXmu(i, k);
      }
      for (int i = 0; i < n; i++) {
         dx(i, k) = (1. / double(n) * gamma(0, k) * fIvar(0, k)) *
                    (n * dout(i, k) - npSumDy - fXmu(i, k) / (fVar(0, k) + epsilon) * npSumDyHMu);
      }
   }
   # endif
}

//______________________________________________________________________________
template <typename Architecture_t>
void TBatchNormLayer<Architecture_t>::Print() const
{
   std::cout << " BATCH NORM Layer: \t";
   std::cout << " Input/Output = ( " ;
   auto &shape = this->GetOutput().GetShape();
   for (size_t i = 0; i < shape.size(); ++i) {
      if (i > 0) std::cout << " , ";
      std::cout << shape[i];
   }
   std::cout  << " ) ";
   std::cout << "\t Norm dim =" << std::setw(6) << this->GetWeightsAt(0).GetNcols();
   std::cout << "\t axis = " << fNormAxis << std::endl;
   std::cout << std::endl;
}

//______________________________________________________________________________

template <typename Architecture_t>
void TBatchNormLayer<Architecture_t>::AddWeightsXMLTo(void *parent)
{

   // write layer width activation function + weigbht and bias matrices

   auto layerxml = gTools().xmlengine().NewChild(parent, 0, "BatchNormLayer");

   //gTools().AddAttr(layerxml, "InputSize", fInputWidth);
   // gTools().xmlengine().NewAttr(layerxml, 0, "Momentum", gTools().StringFromDouble(fMomentum));
   // gTools().xmlengine().NewAttr(layerxml, 0, "Epsilon", gTools().StringFromDouble(fEpsilon));

   gTools().AddAttr(layerxml, "Momentum", fMomentum);
   gTools().AddAttr(layerxml, "Epsilon", fEpsilon);

   // write stored mean and variances
   //using Scalar_t = typename Architecture_t::Scalar_t;

   this->WriteMatrixToXML(layerxml, "Training-mu", fMu_Training);
   this->WriteMatrixToXML(layerxml, "Training-variance", fVar_Training);

   // TMatrixT<Scalar_t> muMat(1, fMu_Training.size(), fMu_Training.data());
   // //VGeneralLayer<TMVA::DNN::TReference<Scalar_t> >::WriteMatrixToXML(layerxml, "Training-mu", muMat);
   // this->WriteMatrixToXML(layerxml, "Training-mu", Matrix_t(muMat) );
   // TMatrixT<Scalar_t> varMat(1, fVar_Training.size(), fVar_Training.data());
   // //VGeneralLayer<TMVA::DNN::TReference<Scalar_t> >::WriteMatrixToXML(layerxml, "Training-variance", varMat);
   // this->WriteMatrixToXML(layerxml, "Training-variance", Matrix_t(varMat) );

   // write weights (gamma and beta)
   this->WriteMatrixToXML(layerxml, "Gamma", this->GetWeightsAt(0));
   this->WriteMatrixToXML(layerxml, "Beta", this->GetWeightsAt(1));

}

//______________________________________________________________________________
template <typename Architecture_t>
void TBatchNormLayer<Architecture_t>::ReadWeightsFromXML(void *parent)
{
   // momentum and epsilon can be added after constructing the class
   gTools().ReadAttr(parent, "Momentum", fMomentum);
   gTools().ReadAttr(parent, "Epsilon", fEpsilon);
   // Read layer weights and biases from XML

   this->ReadMatrixXML(parent, "Training-mu", fMu_Training);
   this->ReadMatrixXML(parent, "Training-variance", fVar_Training);

   // Matrix_t muMat(1, fMu_Training.size());
   // this->ReadMatrixXML(parent, "Training-mu", muMat);
   // TMatrixT<Scalar_t> tmp = muMat;
   // std::copy(tmp.GetMatrixArray(), tmp.GetMatrixArray()+fMu_Training.size(), fMu_Training.begin() );

   // Matrix_t varMat(1, fVar_Training.size());
   // this->ReadMatrixXML(parent, "Training-variance", varMat);
   // TMatrixT<Scalar_t> tmp2 = varMat;
   // std::copy(tmp2.GetMatrixArray(), tmp2.GetMatrixArray()+fVar_Training.size(), fVar_Training.begin() );

   this->ReadMatrixXML(parent, "Gamma", this->GetWeightsAt(0));
   this->ReadMatrixXML(parent, "Beta", this->GetWeightsAt(1));
}

} // namespace DNN
} // namespace TMVA

#endif
