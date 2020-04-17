#include "gtest/gtest.h"

#include <ROOT/RCluster.hxx>
#include <ROOT/RClusterPool.hxx>
#include <ROOT/RColumn.hxx>
#include <ROOT/RColumnModel.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RPage.hxx>
#include <ROOT/RPageStorage.hxx>
#include <ROOT/RPageStorageFile.hxx>
#include <ROOT/RStringView.hxx>

#include <memory>
#include <utility>
#include <vector>

using ClusterSize_t = ROOT::Experimental::ClusterSize_t;
using RCluster = ROOT::Experimental::Detail::RCluster;
using RClusterPool = ROOT::Experimental::Detail::RClusterPool;
using RHeapCluster = ROOT::Experimental::Detail::RHeapCluster;
using RNTupleDescriptor = ROOT::Experimental::RNTupleDescriptor;
using RNTupleVersion = ROOT::Experimental::RNTupleVersion;
using ROnDiskPage = ROOT::Experimental::Detail::ROnDiskPage;
using RPage = ROOT::Experimental::Detail::RPage;
using RPageSource = ROOT::Experimental::Detail::RPageSource;

namespace {

/**
 * An RAII wrapper around an open temporary file on disk. It cleans up the guarded file when the wrapper object
 * goes out of scope.
 */
class FileRaii {
private:
   std::string fPath;
public:
   explicit FileRaii(const std::string &path) : fPath(path) { }
   FileRaii(const FileRaii&) = delete;
   FileRaii& operator=(const FileRaii&) = delete;
   ~FileRaii() { std::remove(fPath.c_str()); }
   std::string GetPath() const { return fPath; }
};

/**
 * Used to track LoadCluster calls triggered by ClusterPool::GetCluster
 */
class RPageSourceMock : public RPageSource {
protected:
   RNTupleDescriptor AttachImpl() final { return RNTupleDescriptor(); }

public:
   /// Records the cluster IDs requests by LoadCluster() calls
   std::vector<ROOT::Experimental::DescriptorId_t> fLoadRequests;

   RPageSourceMock() : RPageSource("test", ROOT::Experimental::RNTupleReadOptions()) {
      ROOT::Experimental::RNTupleDescriptorBuilder descBuilder;
      descBuilder.AddCluster(0, RNTupleVersion(), 0, ClusterSize_t(1));
      descBuilder.AddCluster(1, RNTupleVersion(), 1, ClusterSize_t(1));
      descBuilder.AddCluster(2, RNTupleVersion(), 2, ClusterSize_t(1));
      descBuilder.AddCluster(3, RNTupleVersion(), 3, ClusterSize_t(1));
      descBuilder.AddCluster(4, RNTupleVersion(), 4, ClusterSize_t(1));
      fDescriptor = descBuilder.MoveDescriptor();
   }
   std::unique_ptr<RPageSource> Clone() const final { return nullptr; }
   RPage PopulatePage(ColumnHandle_t, ROOT::Experimental::NTupleSize_t) final { return RPage(); }
   RPage PopulatePage(ColumnHandle_t, const ROOT::Experimental::RClusterIndex &) final { return RPage(); }
   void ReleasePage(RPage &) final {}
   std::unique_ptr<RCluster> LoadCluster(ROOT::Experimental::DescriptorId_t clusterId) final {
      fLoadRequests.push_back(clusterId);
      return std::make_unique<RCluster>(nullptr, clusterId);
   }
};

} // anonymous namespace


TEST(Cluster, Allocate)
{
   auto cluster = new RHeapCluster(nullptr, 0);
   delete cluster;

   auto memory = new char[1];
   cluster = new RHeapCluster(memory, 0);
   delete cluster;
}


TEST(Cluster, Basics)
{
   auto memory = new char[3];
   auto cluster = std::make_unique<RHeapCluster>(memory, 0);
   cluster->Insert(ROnDiskPage::Key(5, 0), ROnDiskPage(&memory[0], 1));
   cluster->Insert(ROnDiskPage::Key(5, 1), ROnDiskPage(&memory[1], 2));

   EXPECT_EQ(nullptr, cluster->GetOnDiskPage(ROnDiskPage::Key(5, 2)));
   EXPECT_EQ(nullptr, cluster->GetOnDiskPage(ROnDiskPage::Key(4, 0)));
   auto onDiskPage = cluster->GetOnDiskPage(ROnDiskPage::Key(5, 0));
   EXPECT_EQ(&memory[0], onDiskPage->GetAddress());
   EXPECT_EQ(1U, onDiskPage->GetSize());
   onDiskPage = cluster->GetOnDiskPage(ROnDiskPage::Key(5, 1));
   EXPECT_EQ(&memory[1], onDiskPage->GetAddress());
   EXPECT_EQ(2U, onDiskPage->GetSize());
}

TEST(ClusterPool, Windows)
{
   EXPECT_DEATH(RClusterPool(nullptr, 0), ".*");
   RClusterPool c1(nullptr, 1);
   EXPECT_EQ(0U, c1.GetWindowPre());
   EXPECT_EQ(1U, c1.GetWindowPost());
   RClusterPool c2(nullptr, 2);
   EXPECT_EQ(0U, c2.GetWindowPre());
   EXPECT_EQ(2U, c2.GetWindowPost());
   RClusterPool c3(nullptr, 3);
   EXPECT_EQ(1U, c3.GetWindowPre());
   EXPECT_EQ(2U, c3.GetWindowPost());
   RClusterPool c5(nullptr, 5);
   EXPECT_EQ(1U, c5.GetWindowPre());
   EXPECT_EQ(4U, c5.GetWindowPost());
   RClusterPool c6(nullptr, 6);
   EXPECT_EQ(2U, c6.GetWindowPre());
   EXPECT_EQ(4U, c6.GetWindowPost());
   RClusterPool c9(nullptr, 9);
   EXPECT_EQ(2U, c9.GetWindowPre());
   EXPECT_EQ(7U, c9.GetWindowPost());
   RClusterPool c10(nullptr, 10);
   EXPECT_EQ(3U, c10.GetWindowPre());
   EXPECT_EQ(7U, c10.GetWindowPost());
   RClusterPool c15(nullptr, 15);
   EXPECT_EQ(3U,  c15.GetWindowPre());
   EXPECT_EQ(12U, c15.GetWindowPost());
   RClusterPool c16(nullptr, 16);
   EXPECT_EQ(4U,  c16.GetWindowPre());
   EXPECT_EQ(12U, c16.GetWindowPost());
}

TEST(ClusterPool, GetCluster)
{
   RPageSourceMock p1;
   RClusterPool c1(&p1, 1);
   c1.GetCluster(3);
   ASSERT_EQ(1U, p1.fLoadRequests.size());
   EXPECT_EQ(3U, p1.fLoadRequests[0]);

   RPageSourceMock p2;
   {
      RClusterPool c2(&p2, 2);
      c2.GetCluster(0);
   }
   ASSERT_EQ(2U, p2.fLoadRequests.size());
   EXPECT_EQ(0U, p2.fLoadRequests[0]);
   EXPECT_EQ(1U, p2.fLoadRequests[1]);

   RPageSourceMock p3;
   {
      RClusterPool c3(&p3, 4);
      c3.GetCluster(2);
   }
   ASSERT_EQ(3U, p3.fLoadRequests.size());
   EXPECT_EQ(2U, p3.fLoadRequests[0]);
   EXPECT_EQ(3U, p3.fLoadRequests[1]);
   EXPECT_EQ(4U, p3.fLoadRequests[2]);
}


TEST(PageStorageFile, LoadCluster)
{
   FileRaii fileGuard("test_ntuple_clusters.root");

   auto modelWrite = ROOT::Experimental::RNTupleModel::Create();
   auto wrPt = modelWrite->MakeField<float>("pt", 42.0);
   auto wrTag = modelWrite->MakeField<int32_t>("tag", 0);

   {
      ROOT::Experimental::RNTupleWriter ntuple(
         std::move(modelWrite), std::make_unique<ROOT::Experimental::Detail::RPageSinkFile>(
            "myNTuple", fileGuard.GetPath(), ROOT::Experimental::RNTupleWriteOptions()));
      ntuple.Fill();
      ntuple.CommitCluster();
      *wrPt = 24.0;
      *wrTag = 1;
      ntuple.Fill();
   }

   ROOT::Experimental::Detail::RPageSourceFile source(
      "myNTuple", fileGuard.GetPath(), ROOT::Experimental::RNTupleReadOptions());
   source.Attach();

   auto cluster = source.LoadCluster(0);
   EXPECT_EQ(0U, cluster->GetId());
   EXPECT_EQ(0U, cluster->GetNOnDiskPages());

   auto ptId = source.GetDescriptor().FindFieldId("pt");
   EXPECT_NE(ROOT::Experimental::kInvalidDescriptorId, ptId);
   auto colId = source.GetDescriptor().FindColumnId(ptId, 0);
   EXPECT_NE(ROOT::Experimental::kInvalidDescriptorId, colId);

   auto column = std::unique_ptr<ROOT::Experimental::Detail::RColumn>(
      ROOT::Experimental::Detail::RColumn::Create<float, ROOT::Experimental::EColumnType::kReal32>(
         ROOT::Experimental::RColumnModel(ROOT::Experimental::EColumnType::kReal32, false), 0));
   column->Connect(ptId, &source);
   cluster = source.LoadCluster(1);
   EXPECT_EQ(1U, cluster->GetId());
   EXPECT_EQ(1U, cluster->GetNOnDiskPages());

   ROnDiskPage::Key key(colId, 0);
   EXPECT_NE(nullptr, cluster->GetOnDiskPage(key));
}
