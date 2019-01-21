#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>

#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <ROOT/TTreeProcessorMT.hxx>

#include "gtest/gtest.h"

void WriteFiles(const std::string &treename, const std::vector<std::string> &filenames)
{
   int v = 0;
   for (const auto &f : filenames) {
      TFile file(f.c_str(), "recreate");
      TTree t(treename.c_str(), treename.c_str());
      t.Branch("v", &v);
      for (auto i = 0; i < 10; ++i) {
         ++v;
         t.Fill();
      }
      t.Write();
   }
}

void WriteFileManyClusters(unsigned int nevents, const char *treename, const char *filename)
{
   int v = 0;
   TFile file(filename, "recreate");
   TTree t(treename, treename);
   t.Branch("v", &v);
   //t.SetAutoFlush(1);
   for (auto i = 0U; i < nevents; ++i) {
      t.Fill();
      t.FlushBaskets();
   }
   t.Write();
   file.Close();
}

   void DeleteFiles(const std::vector<std::string> &filenames)
   {
      for (const auto &f : filenames)
         gSystem->Unlink(f.c_str());
   }

   TEST(TreeProcessorMT, EmptyTChain)
   {
      TChain c("mytree");
      auto exceptionFired(false);
      try {
         ROOT::TTreeProcessorMT proc(c);
      } catch (...) {
         exceptionFired = true;
      }
      EXPECT_TRUE(exceptionFired);
   }

   TEST(TreeProcessorMT, ManyFiles)
   {
      const auto nFiles = 100u;
      const std::string treename = "t";
      std::vector<std::string> filenames;
      for (auto i = 0u; i < nFiles; ++i)
         filenames.emplace_back("treeprocmt_" + std::to_string(i) + ".root");

      WriteFiles(treename, filenames);

      std::atomic_int sum(0);
      std::atomic_int count(0);
      auto sumValues = [&sum, &count](TTreeReader &r) {
         TTreeReaderValue<int> v(r, "v");
         std::random_device seed;
         std::default_random_engine eng(seed());
         std::uniform_int_distribution<> rand(1, 100);
         while (r.Next()) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(rand(eng)));
            sum += *v;
            ++count;
         }
      };

      // TTreeProcMT requires a vector<string_view>
      std::vector<std::string_view> fnames;
      for (const auto &f : filenames)
         fnames.emplace_back(f);

      ROOT::TTreeProcessorMT proc(fnames, treename);
      proc.Process(sumValues);

      EXPECT_EQ(count.load(), int(nFiles * 10)); // 10 entries per file
      EXPECT_EQ(sum.load(), 500500);             // sum 1..nFiles*10

      DeleteFiles(filenames);
   }

   TEST(TreeProcessorMT, TreeInSubDirectory)
   {
      auto filename = "fileTreeInSubDirectory.root";
      auto procLambda = [](TTreeReader &r) {
         while (r.Next())
            ;
      };

      {
         TFile f(filename, "RECREATE");
         auto dir0 = f.mkdir("dir0");
         dir0->cd();
         auto dir1 = dir0->mkdir("dir1");
         dir1->cd();
         TTree t("tree", "tree");
         t.Write();
      }

      ROOT::EnableThreadSafety();

      auto fullPath = "dir0/dir1/tree";

      // With a TTree
      TFile f(filename);
      auto t = (TTree *)f.Get(fullPath);
      ROOT::TTreeProcessorMT tp(*t);
      tp.Process(procLambda);

      // With a TChain
      std::string chainElementName = filename;
      chainElementName += "/";
      chainElementName += fullPath;
      TChain chain;
      chain.Add(chainElementName.c_str());
      ROOT::TTreeProcessorMT tpc(chain);
      tpc.Process(procLambda);

      gSystem->Unlink(filename);
   }

TEST(TreeProcessorMT, MaxTasks)
{
   const auto nEvents = 991;
   const auto filename = "TreeProcessorMT_MaxTasks.root";
   const auto treename = "t";
   WriteFileManyClusters(nEvents, treename, filename);
   auto nTasks = 0U;
   std::map<unsigned int, unsigned int> nEntriesCountsMap;
   std::mutex theMutex;
   auto f = [&](TTreeReader &t) {
      auto nentries = 0U;
      while(t.Next()) nentries++;
      std::lock_guard<std::mutex> lg(theMutex);
      nTasks++;
      if(!nEntriesCountsMap.insert({nentries, 1U}).second) {
         nEntriesCountsMap[nentries]++;
      }
   };

   ROOT::DisableImplicitMT();
   ROOT::EnableImplicitMT(4);

   ROOT::TTreeProcessorMT p(filename, treename);
   p.Process(f);

   EXPECT_EQ(nTasks, 96U) << "Wrong number of tasks generated!\n";
   EXPECT_EQ(nEntriesCountsMap[10], 65U) << "Wrong number of tasks with 10 clusters each!\n";
   EXPECT_EQ(nEntriesCountsMap[11], 31U) << "Wrong number of tasks with 11 clusters each!\n";

   gSystem->Unlink(filename);
   ROOT::DisableImplicitMT();
}