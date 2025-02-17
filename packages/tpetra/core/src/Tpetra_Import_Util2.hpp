// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef TPETRA_IMPORT_UTIL2_HPP
#define TPETRA_IMPORT_UTIL2_HPP

///
/// \file Tpetra_Import_Util2.hpp
/// \brief Utility functions for packing and unpacking sparse matrix entries.
///

#include "Tpetra_ConfigDefs.hpp"
#include "Tpetra_Import.hpp"
#include "Tpetra_HashTable.hpp"
#include "Tpetra_Map.hpp"
#include "Tpetra_Util.hpp"
#include "Tpetra_Distributor.hpp"
#include "Tpetra_Details_reallocDualViewIfNeeded.hpp"
#include "Tpetra_Details_MpiTypeTraits.hpp"
#include "Tpetra_Vector.hpp"
#include "Kokkos_DualView.hpp"
#include "KokkosSparse_SortCrs.hpp"
#include <Teuchos_Array.hpp>
#include "Tpetra_Details_createMirrorView.hpp"
#include <Kokkos_UnorderedMap.hpp>
#include <unordered_map>
#include <utility>
#include <set>

#include "Tpetra_CrsMatrix_decl.hpp"

#include <Kokkos_Core.hpp>
#include <Kokkos_Sort.hpp>

namespace Tpetra {
namespace Import_Util {

/// \brief Sort the entries of the (raw CSR) matrix by column index
///   within each row.
template<typename Scalar, typename Ordinal>
void
sortCrsEntries (const Teuchos::ArrayView<size_t>& CRS_rowptr,
                const Teuchos::ArrayView<Ordinal>& CRS_colind,
                const Teuchos::ArrayView<Scalar>&CRS_vals);

template<typename Ordinal>
void
sortCrsEntries (const Teuchos::ArrayView<size_t>& CRS_rowptr,
                const Teuchos::ArrayView<Ordinal>& CRS_colind);

template<typename rowptr_array_type, typename colind_array_type, typename vals_array_type>
void
sortCrsEntries (const rowptr_array_type& CRS_rowptr,
                const colind_array_type& CRS_colind,
                const vals_array_type& CRS_vals);

template<typename rowptr_array_type, typename colind_array_type>
void
sortCrsEntries (const rowptr_array_type& CRS_rowptr,
                const colind_array_type& CRS_colind);

/// \brief Sort and merge the entries of the (raw CSR) matrix by
///   column index within each row.
///
/// Entries with the same column index get merged additively.
template<typename Scalar, typename Ordinal>
void
sortAndMergeCrsEntries (const Teuchos::ArrayView<size_t>& CRS_rowptr,
                        const Teuchos::ArrayView<Ordinal>& CRS_colind,
                        const Teuchos::ArrayView<Scalar>& CRS_vals);

template<typename Ordinal>
void
sortAndMergeCrsEntries (const Teuchos::ArrayView<size_t>& CRS_rowptr,
                        const Teuchos::ArrayView<Ordinal>& CRS_colind);

template<class rowptr_view_type, class colind_view_type, class vals_view_type>
void
sortAndMergeCrsEntries (const rowptr_view_type& CRS_rowptr,
		        const colind_view_type& CRS_colind,
		        const vals_view_type& CRS_vals);

/// \brief lowCommunicationMakeColMapAndReindex
///
/// If you know the owning PIDs already, you can make the colmap a lot
/// less expensively.  If LocalOrdinal and GlobalOrdinal are the same,
/// you can (and should) use the same array for both columnIndices_LID
/// and columnIndices_GID.  This routine works just fine "in place."
///
/// Note: The owningPids vector (on input) should contain owning PIDs
/// for each entry in the matrix, like that generated by
/// Tpetra::Import_Util::unpackAndCombineIntoCrsArrays routine.  Note:
/// This method will return a Teuchos::Array of the remotePIDs, used for
/// construction of the importer.
///
/// \warning This method is intended for expert developer use only,
///   and should never be called by user code.
template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void
lowCommunicationMakeColMapAndReindex (
                                      const Teuchos::ArrayView<const size_t> &rowptr,
                                      const Teuchos::ArrayView<LocalOrdinal> &colind_LID,
                                      const Teuchos::ArrayView<GlobalOrdinal> &colind_GID,
                                      const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& domainMapRCP,
                                      const Teuchos::ArrayView<const int> &owningPIDs,
                                      Teuchos::Array<int> &remotePIDs,
                                      Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & colMap);

/// \brief lowCommunicationMakeColMapAndReindex
///
/// Accepts Kokkos::View's for compatibility with non-Serial Node path in the Transfer and FillComplete
/// path in Tpetra::CrsMatrix, thus avoiding unnecessary deep_copy's.
template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void
lowCommunicationMakeColMapAndReindex (
                                      const Kokkos::View<size_t*,typename Node::device_type>        rowptr_view,
                                      const Kokkos::View<LocalOrdinal*,typename Node::device_type>  colind_LID_view,
                                      const Kokkos::View<GlobalOrdinal*,typename Node::device_type> colind_GID_view,
                                      const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& domainMapRCP,
                                      const Teuchos::ArrayView<const int> &owningPIDs,
                                      Teuchos::Array<int> &remotePIDs,
                                      Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & colMap);

  /// \brief Generates an list of owning PIDs based on two transfer (aka import/export objects)
  /// Let:
  ///   OwningMap = useReverseModeForOwnership ? transferThatDefinesOwnership.getTargetMap() : transferThatDefinesOwnership.getSourceMap();
  ///   MapAo     = useReverseModeForOwnership ? transferThatDefinesOwnership.getSourceMap() : transferThatDefinesOwnership.getTargetMap();
  ///   MapAm     = useReverseModeForMigration ? transferThatDefinesMigration.getTargetMap() : transferThatDefinesMigration.getSourceMap();
  ///   VectorMap = useReverseModeForMigration ? transferThatDefinesMigration.getSourceMap() : transferThatDefinesMigration.getTargetMap();
  /// Precondition:
  ///  1) MapAo.isSameAs(*MapAm)                      - map compatibility between transfers
  ///  2) VectorMap->isSameAs(*owningPIDs->getMap())  - map compabibility between transfer & vector
  ///  3) OwningMap->isOneToOne()                     - owning map is 1-to-1
  ///  --- Precondition 3 is only checked in DEBUG mode ---
  /// Postcondition:
  ///   owningPIDs[VectorMap->getLocalElement(GID i)] =   j iff  (OwningMap->isLocalElement(GID i) on rank j)
  template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
  void getTwoTransferOwnershipVector(const ::Tpetra::Details::Transfer<LocalOrdinal, GlobalOrdinal, Node>& transferThatDefinesOwnership,
                                     bool useReverseModeForOwnership,
                                     const ::Tpetra::Details::Transfer<LocalOrdinal, GlobalOrdinal, Node>& transferForMigratingData,
                                     bool useReverseModeForMigration,
                                     Tpetra::Vector<int,LocalOrdinal,GlobalOrdinal,Node> & owningPIDs);

} // namespace Import_Util
} // namespace Tpetra


//
// Implementations
//

namespace Tpetra {
namespace Import_Util {


template<typename PID, typename GlobalOrdinal>
bool sort_PID_then_GID(const std::pair<PID,GlobalOrdinal> &a,
                       const std::pair<PID,GlobalOrdinal> &b)
{
  if(a.first!=b.first)
    return (a.first < b.first);
  return (a.second < b.second);
}

template<typename PID,
         typename GlobalOrdinal,
         typename LocalOrdinal>
bool sort_PID_then_pair_GID_LID(const std::pair<PID, std::pair< GlobalOrdinal, LocalOrdinal > > &a,
                                const std::pair<PID, std::pair< GlobalOrdinal, LocalOrdinal > > &b)
{
  if(a.first!=b.first)
    return a.first < b.first;
  else
    return (a.second.first < b.second.first);
}

template<typename Scalar,
         typename LocalOrdinal,
         typename GlobalOrdinal,
         typename Node>
void
reverseNeighborDiscovery(const CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>  & SourceMatrix,
                         const typename CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>::row_ptrs_host_view_type & rowptr,
                         const typename CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>::local_inds_host_view_type & colind,
                         const Tpetra::Details::Transfer<LocalOrdinal,GlobalOrdinal,Node>& RowTransfer,
                         Teuchos::RCP<const Tpetra::Import<LocalOrdinal,GlobalOrdinal,Node> > MyImporter,
                         Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > MyDomainMap,
                         Teuchos::ArrayRCP<int>& type3PIDs,
                         Teuchos::ArrayRCP<LocalOrdinal>& type3LIDs,
                         Teuchos::RCP<const Teuchos::Comm<int> >& rcomm)
{
#ifdef HAVE_TPETRACORE_MPI
    using Teuchos::TimeMonitor;
    using ::Tpetra::Details::Behavior;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef std::pair<GO,GO> pidgidpair_t;
    using Teuchos::RCP;
    const std::string prefix {" Import_Util2::ReverseND:: "};
    const std::string label ("IU2::Neighbor");

    // There can be no neighbor discovery if you don't have an importer
    if(MyImporter.is_null()) return;

    std::ostringstream errstr;
    bool error = false;
    auto const comm             = MyDomainMap->getComm();

    MPI_Comm rawComm            = getRawMpiComm(*comm);
    const int MyPID             = rcomm->getRank ();

    // Things related to messages I am sending in forward mode (RowTransfer)
    // *** Note: this will be incorrect for transferAndFillComplete if it is in reverse mode. FIXME cbl.
    auto ExportPIDs                 = RowTransfer.getExportPIDs();
    auto ExportLIDs                 = RowTransfer.getExportLIDs();
    auto NumExportLIDs              = RowTransfer.getNumExportIDs();

    Distributor & Distor            = MyImporter->getDistributor();
    const size_t NumRecvs           = Distor.getNumReceives();
    const size_t NumSends           = Distor.getNumSends();
    auto RemoteLIDs                 = MyImporter->getRemoteLIDs();
    auto const ProcsFrom            = Distor.getProcsFrom();
    auto const ProcsTo              = Distor.getProcsTo();

    auto LengthsFrom                = Distor.getLengthsFrom();
    auto MyColMap                   = SourceMatrix.getColMap();
    const size_t numCols            = MyColMap->getLocalNumElements ();
    RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > target = MyImporter->getTargetMap();

    // Get the owning pids in a special way,
    // s.t. ProcsFrom[RemotePIDs[i]] is the proc that owns RemoteLIDs[j]....
    Teuchos::Array<int> RemotePIDOrder(numCols,-1);

    // For each remote ID, record index into ProcsFrom, who owns it.
    for (size_t i = 0, j = 0; i < NumRecvs; ++i) {
      for (size_t k = 0; k < LengthsFrom[i]; ++k) {
        const int pid = ProcsFrom[i];
        if (pid != MyPID) {
          RemotePIDOrder[RemoteLIDs[j]] = i;
        }
        j++;
      }
    }

    // Step One: Start tacking the (GID,PID) pairs on the std sets
    //
    // For each index in ProcsFrom, we will insert into a set of (PID,
    // GID) pairs, in order to build a list of such pairs for each of
    // those processes.  Since this is building a reverse, we will send
    // to these processes.
    Teuchos::Array<int> ReverseSendSizes(NumRecvs,0);
    // do this as C array to avoid Teuchos::Array value initialization of all reserved memory
    Teuchos::Array< Teuchos::ArrayRCP<pidgidpair_t > > RSB(NumRecvs);

    {
#ifdef HAVE_TPETRA_MMM_TIMINGS
        TimeMonitor set_all(*TimeMonitor::getNewTimer(prefix + std::string("isMMallSetRSB")));
#endif

        // 25 Jul 2018: CBL
        // todo:std::unordered_set (hash table),
        // with an adequate prereservation ("bucket count").
        // An onordered_set has to have a custom hasher for pid/gid pair
        // However, when pidsets is copied to RSB, it will be in key
        // order _not_ in pid,gid order. (unlike std::set).
        // Impliment this with a reserve, and time BOTH building pidsets
        // _and_ the sort after the receive. Even if unordered_set saves
        // time, if it causes the sort to be longer, it's not a win.

        Teuchos::Array<std::set<pidgidpair_t>> pidsets(NumRecvs);
        {
#ifdef HAVE_TPETRA_MMM_TIMINGS
            TimeMonitor set_insert(*TimeMonitor::getNewTimer(prefix + std::string("isMMallSetRSBinsert")));
#endif
            for(size_t i=0; i < NumExportLIDs; i++) {
                LO lid = ExportLIDs[i];
                GO exp_pid = ExportPIDs[i];
                for(auto j=rowptr[lid]; j<rowptr[lid+1]; j++){
                    int pid_order = RemotePIDOrder[colind[j]];
                    if(pid_order!=-1) {
                        GO gid = MyColMap->getGlobalElement(colind[j]); //Epetra SM.GCID46 =>sm->graph-> {colmap(colind)}
                        auto tpair = pidgidpair_t(exp_pid,gid);
                        pidsets[pid_order].insert(pidsets[pid_order].end(),tpair);
                    }
                }
            }
        }

        {
#ifdef HAVE_TPETRA_MMM_TIMINGS
            TimeMonitor set_cpy(*TimeMonitor::getNewTimer(prefix + std::string("isMMallSetRSBcpy")));
#endif
            int jj = 0;
            for(auto &&ps : pidsets)  {
                auto s = ps.size();
                RSB[jj] = Teuchos::arcp(new pidgidpair_t[ s ],0, s ,true);
                std::copy(ps.begin(),ps.end(),RSB[jj]);
                ReverseSendSizes[jj]=s;
                ++jj;
            }
        }
    } // end of set based packing.

    Teuchos::Array<int> ReverseRecvSizes(NumSends,-1);
    Teuchos::Array<MPI_Request> rawBreq(ProcsFrom.size()+ProcsTo.size(), MPI_REQUEST_NULL);
    // 25 Jul 2018: MPI_TAG_UB is the largest tag value; could be < 32768.
    const int mpi_tag_base_ = 3;

    int mpireq_idx=0;
    for(int i=0;i<ProcsTo.size();++i) {
        int Rec_Tag = mpi_tag_base_ + ProcsTo[i];
        int * thisrecv = (int *) (&ReverseRecvSizes[i]);
        MPI_Request rawRequest = MPI_REQUEST_NULL;
        MPI_Irecv(const_cast<int*>(thisrecv),
                  1,
                  MPI_INT,
                  ProcsTo[i],
                  Rec_Tag,
                  rawComm,
                  &rawRequest);
        rawBreq[mpireq_idx++]=rawRequest;
    }
    for(int i=0;i<ProcsFrom.size();++i) {
        int Send_Tag = mpi_tag_base_ + MyPID;
        int * mysend = ( int *)(&ReverseSendSizes[i]);
        MPI_Request rawRequest = MPI_REQUEST_NULL;
        MPI_Isend(mysend,
                  1,
                  MPI_INT,
                  ProcsFrom[i],
                  Send_Tag,
                  rawComm,
                  &rawRequest);
        rawBreq[mpireq_idx++]=rawRequest;
    }
    Teuchos::Array<MPI_Status> rawBstatus(rawBreq.size());
#ifdef HAVE_TPETRA_DEBUG
    const int err1 =
#endif
      MPI_Waitall (rawBreq.size(), rawBreq.getRawPtr(),
                   rawBstatus.getRawPtr());


#ifdef HAVE_TPETRA_DEBUG
    if(err1) {
        errstr <<MyPID<< "sE1 reverseNeighborDiscovery Mpi_Waitall error on send ";
        error=true;
        std::cerr<<errstr.str()<<std::flush;
    }
#endif

    int totalexportpairrecsize = 0;
    for (size_t i = 0; i < NumSends; ++i) {
      totalexportpairrecsize += ReverseRecvSizes[i];
#ifdef HAVE_TPETRA_DEBUG
      if(ReverseRecvSizes[i]<0) {
        errstr << MyPID << "E4 reverseNeighborDiscovery: got < 0 for receive size "<<ReverseRecvSizes[i]<<std::endl;
        error=true;
      }
#endif
    }
    Teuchos::ArrayRCP<pidgidpair_t >AllReverseRecv= Teuchos::arcp(new pidgidpair_t[totalexportpairrecsize],0,totalexportpairrecsize,true);
    int offset = 0;
    mpireq_idx=0;
    for(int i=0;i<ProcsTo.size();++i) {
        int recv_data_size =    ReverseRecvSizes[i]*2;
        int recvData_MPI_Tag = mpi_tag_base_*2 + ProcsTo[i];
        MPI_Request rawRequest = MPI_REQUEST_NULL;
        GO * rec_bptr = (GO*) (&AllReverseRecv[offset]);
        offset+=ReverseRecvSizes[i];
        MPI_Irecv(rec_bptr,
                  recv_data_size,
                  ::Tpetra::Details::MpiTypeTraits<GO>::getType(rec_bptr[0]),
                  ProcsTo[i],
                  recvData_MPI_Tag,
                  rawComm,
                  &rawRequest);
        rawBreq[mpireq_idx++]=rawRequest;
    }
    for(int ii=0;ii<ProcsFrom.size();++ii) {
        GO * send_bptr = (GO*) (RSB[ii].getRawPtr());
        MPI_Request rawSequest = MPI_REQUEST_NULL;
        int send_data_size = ReverseSendSizes[ii]*2; // 2 == count of pair
        int sendData_MPI_Tag = mpi_tag_base_*2+MyPID;
        MPI_Isend(send_bptr,
                  send_data_size,
                  ::Tpetra::Details::MpiTypeTraits<GO>::getType(send_bptr[0]),
                  ProcsFrom[ii],
                  sendData_MPI_Tag,
                  rawComm,
                  &rawSequest);

        rawBreq[mpireq_idx++]=rawSequest;
    }
#ifdef HAVE_TPETRA_DEBUG
    const int err =
#endif
      MPI_Waitall (rawBreq.size(),
                   rawBreq.getRawPtr(),
                   rawBstatus.getRawPtr());
#ifdef HAVE_TPETRA_DEBUG
    if(err) {
        errstr <<MyPID<< "E3.r reverseNeighborDiscovery Mpi_Waitall error on receive ";
        error=true;
        std::cerr<<errstr.str()<<std::flush;
    }
#endif
    std::sort(AllReverseRecv.begin(), AllReverseRecv.end(), Tpetra::Import_Util::sort_PID_then_GID<GlobalOrdinal, GlobalOrdinal>);

    auto newEndOfPairs = std::unique(AllReverseRecv.begin(), AllReverseRecv.end());
// don't resize to remove non-unique, just use the end-of-unique iterator
    if(AllReverseRecv.begin() == newEndOfPairs)  return;
    int ARRsize = std::distance(AllReverseRecv.begin(),newEndOfPairs);
    auto rPIDs = Teuchos::arcp(new int[ARRsize],0,ARRsize,true);
    auto rLIDs = Teuchos::arcp(new LocalOrdinal[ARRsize],0,ARRsize,true);

    int tsize=0;
    for(auto itr = AllReverseRecv.begin();  itr!=newEndOfPairs; ++itr ) {
        if((int)(itr->first) != MyPID) {
            rPIDs[tsize]=(int)itr->first;
            LocalOrdinal lid = MyDomainMap->getLocalElement(itr->second);
            rLIDs[tsize]=lid;
            tsize++;
        }
    }

    type3PIDs=rPIDs.persistingView(0,tsize);
    type3LIDs=rLIDs.persistingView(0,tsize);

    if(error){
        std::cerr<<errstr.str()<<std::flush;
        comm->barrier();
        comm->barrier();
        comm->barrier();
        MPI_Abort (MPI_COMM_WORLD, -1);
    }
#endif
}

// Note: This should get merged with the other Tpetra sort routines eventually.
template<typename Scalar, typename Ordinal>
void
sortCrsEntries (const Teuchos::ArrayView<size_t> &CRS_rowptr,
                const Teuchos::ArrayView<Ordinal> & CRS_colind,
                const Teuchos::ArrayView<Scalar> &CRS_vals)
{
  // For each row, sort column entries from smallest to largest.
  // Use shell sort. Stable sort so it is fast if indices are already sorted.
  // Code copied from  Epetra_CrsMatrix::SortEntries()
  size_t NumRows = CRS_rowptr.size()-1;
  size_t nnz = CRS_colind.size();

  const bool permute_values_array = CRS_vals.size() > 0;

  for(size_t i = 0; i < NumRows; i++){
    size_t start=CRS_rowptr[i];
    if(start >= nnz) continue;

    size_t NumEntries   = CRS_rowptr[i+1] - start;
    Teuchos::ArrayRCP<Scalar> locValues;
    if (permute_values_array)
      locValues = Teuchos::arcp<Scalar>(&CRS_vals[start], 0, NumEntries, false);
    Teuchos::ArrayRCP<Ordinal> locIndices(&CRS_colind[start], 0, NumEntries, false);

    Ordinal n = NumEntries;
    Ordinal m = 1;
    while (m<n) m = m*3+1;
    m /= 3;

    while(m > 0) {
      Ordinal max = n - m;
      for(Ordinal j = 0; j < max; j++) {
        for(Ordinal k = j; k >= 0; k-=m) {
          if(locIndices[k+m] >= locIndices[k])
            break;
          if (permute_values_array) {
            Scalar dtemp = locValues[k+m];
            locValues[k+m] = locValues[k];
            locValues[k] = dtemp;
          }
          Ordinal itemp = locIndices[k+m];
          locIndices[k+m] = locIndices[k];
          locIndices[k] = itemp;
        }
      }
      m = m/3;
    }
  }
}

template<typename Ordinal>
void
sortCrsEntries (const Teuchos::ArrayView<size_t> &CRS_rowptr,
                const Teuchos::ArrayView<Ordinal> & CRS_colind)
{
  // Generate dummy values array
  Teuchos::ArrayView<Tpetra::Details::DefaultTypes::scalar_type> CRS_vals;
  sortCrsEntries (CRS_rowptr, CRS_colind, CRS_vals);
}

namespace Impl {

template<class RowOffsetsType, class ColumnIndicesType, class ValuesType>
class SortCrsEntries {
private:
  typedef typename ColumnIndicesType::non_const_value_type ordinal_type;
  typedef typename ValuesType::non_const_value_type scalar_type;

public:
  SortCrsEntries (const RowOffsetsType& ptr,
                  const ColumnIndicesType& ind,
                  const ValuesType& val) :
    ptr_ (ptr),
    ind_ (ind),
    val_ (val)
  {
    static_assert (std::is_signed<ordinal_type>::value, "The type of each "
                   "column index -- that is, the type of each entry of ind "
                   "-- must be signed in order for this functor to work.");
  }

  KOKKOS_FUNCTION void operator() (const size_t i) const
  {
    const size_t nnz = ind_.extent (0);
    const size_t start = ptr_(i);
    const bool permute_values_array = val_.extent(0) > 0;

    if (start < nnz) {
      const size_t NumEntries = ptr_(i+1) - start;

      const ordinal_type n = static_cast<ordinal_type> (NumEntries);
      ordinal_type m = 1;
      while (m<n) m = m*3+1;
      m /= 3;

      while (m > 0) {
        ordinal_type max = n - m;
        for (ordinal_type j = 0; j < max; j++) {
          for (ordinal_type k = j; k >= 0; k -= m) {
            const size_t sk = start+k;
            if (ind_(sk+m) >= ind_(sk)) {
              break;
            }
            if (permute_values_array) {
              const scalar_type dtemp = val_(sk+m);
              val_(sk+m)   = val_(sk);
              val_(sk)     = dtemp;
            }
            const ordinal_type itemp = ind_(sk+m);
            ind_(sk+m) = ind_(sk);
            ind_(sk)   = itemp;
          }
        }
        m = m/3;
      }
    }
  }

  static void
  sortCrsEntries (const RowOffsetsType& ptr,
                  const ColumnIndicesType& ind,
                  const ValuesType& val)
  {
    // For each row, sort column entries from smallest to largest.
    // Use shell sort. Stable sort so it is fast if indices are already sorted.
    // Code copied from  Epetra_CrsMatrix::SortEntries()
    // NOTE: This should not be taken as a particularly efficient way to sort
    // rows of matrices in parallel.  But it is correct, so that's something.
    if (ptr.extent (0) == 0) {
      return; // no rows, so nothing to sort
    }
    const size_t NumRows = ptr.extent (0) - 1;

    typedef size_t index_type; // what this function was using; not my choice
    typedef typename ValuesType::execution_space execution_space;
    // Specify RangePolicy explicitly, in order to use correct execution
    // space.  See #1345.
    typedef Kokkos::RangePolicy<execution_space, index_type> range_type;
    Kokkos::parallel_for ("sortCrsEntries", range_type (0, NumRows),
      SortCrsEntries (ptr, ind, val));
  }

private:
  RowOffsetsType ptr_;
  ColumnIndicesType ind_;
  ValuesType val_;
};

} // namespace Impl

template<typename rowptr_array_type, typename colind_array_type, typename vals_array_type>
void
sortCrsEntries (const rowptr_array_type& CRS_rowptr,
                const colind_array_type& CRS_colind,
                const vals_array_type& CRS_vals)
{
  Impl::SortCrsEntries<rowptr_array_type, colind_array_type,
    vals_array_type>::sortCrsEntries (CRS_rowptr, CRS_colind, CRS_vals);
}

template<typename rowptr_array_type, typename colind_array_type>
void
sortCrsEntries (const rowptr_array_type& CRS_rowptr,
                const colind_array_type& CRS_colind)
{
  // Generate dummy values array
  typedef typename colind_array_type::execution_space execution_space;
  typedef Tpetra::Details::DefaultTypes::scalar_type scalar_type;
  typedef typename Kokkos::View<scalar_type*, execution_space> scalar_view_type;
  scalar_view_type CRS_vals;
  sortCrsEntries<rowptr_array_type, colind_array_type,
    scalar_view_type>(CRS_rowptr, CRS_colind, CRS_vals);
}

// Note: This should get merged with the other Tpetra sort routines eventually.
template<typename Scalar, typename Ordinal>
void
sortAndMergeCrsEntries (const Teuchos::ArrayView<size_t> &CRS_rowptr,
                        const Teuchos::ArrayView<Ordinal> & CRS_colind,
                        const Teuchos::ArrayView<Scalar> &CRS_vals)
{
  // For each row, sort column entries from smallest to largest,
  // merging column ids that are identify by adding values.  Use shell
  // sort. Stable sort so it is fast if indices are already sorted.
  // Code copied from Epetra_CrsMatrix::SortEntries()

  if (CRS_rowptr.size () == 0) {
    return; // no rows, so nothing to sort
  }
  const size_t NumRows = CRS_rowptr.size () - 1;
  const size_t nnz = CRS_colind.size ();
  size_t new_curr = CRS_rowptr[0];
  size_t old_curr = CRS_rowptr[0];

  const bool permute_values_array = CRS_vals.size() > 0;

  for(size_t i = 0; i < NumRows; i++){
    const size_t old_rowptr_i=CRS_rowptr[i];
    CRS_rowptr[i] = old_curr;
    if(old_rowptr_i >= nnz) continue;

    size_t NumEntries   = CRS_rowptr[i+1] - old_rowptr_i;
    Teuchos::ArrayRCP<Scalar> locValues;
    if (permute_values_array)
      locValues = Teuchos::arcp<Scalar>(&CRS_vals[old_rowptr_i], 0, NumEntries, false);
    Teuchos::ArrayRCP<Ordinal> locIndices(&CRS_colind[old_rowptr_i], 0, NumEntries, false);

    // Sort phase
    Ordinal n = NumEntries;
    Ordinal m = n/2;

    while(m > 0) {
      Ordinal max = n - m;
      for(Ordinal j = 0; j < max; j++) {
        for(Ordinal k = j; k >= 0; k-=m) {
          if(locIndices[k+m] >= locIndices[k])
            break;
          if (permute_values_array) {
            Scalar dtemp = locValues[k+m];
            locValues[k+m] = locValues[k];
            locValues[k] = dtemp;
          }
          Ordinal itemp = locIndices[k+m];
          locIndices[k+m] = locIndices[k];
          locIndices[k] = itemp;
        }
      }
      m = m/2;
    }

    // Merge & shrink
    for(size_t j=old_rowptr_i; j < CRS_rowptr[i+1]; j++) {
      if(j > old_rowptr_i && CRS_colind[j]==CRS_colind[new_curr-1]) {
        if (permute_values_array) CRS_vals[new_curr-1] += CRS_vals[j];
      }
      else if(new_curr==j) {
        new_curr++;
      }
      else {
        CRS_colind[new_curr] = CRS_colind[j];
        if (permute_values_array) CRS_vals[new_curr]   = CRS_vals[j];
        new_curr++;
      }
    }
    old_curr=new_curr;
  }

  CRS_rowptr[NumRows] = new_curr;
}

template<typename Ordinal>
void
sortAndMergeCrsEntries (const Teuchos::ArrayView<size_t> &CRS_rowptr,
                        const Teuchos::ArrayView<Ordinal> & CRS_colind)
{
  Teuchos::ArrayView<Tpetra::Details::DefaultTypes::scalar_type> CRS_vals;
  return sortAndMergeCrsEntries(CRS_rowptr, CRS_colind, CRS_vals);
}

template<class rowptr_view_type, class colind_view_type, class vals_view_type>
void
sortAndMergeCrsEntries(rowptr_view_type& CRS_rowptr,
		       colind_view_type& CRS_colind,
		       vals_view_type& CRS_vals)
{
  using execution_space = typename vals_view_type::execution_space;

  auto CRS_rowptr_in = CRS_rowptr;
  auto CRS_colind_in = CRS_colind;
  auto CRS_vals_in   = CRS_vals;

  KokkosSparse::sort_and_merge_matrix<execution_space, rowptr_view_type,
				      colind_view_type, vals_view_type>(CRS_rowptr_in, CRS_colind_in, CRS_vals_in,
									CRS_rowptr, CRS_colind, CRS_vals);
}


template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void
lowCommunicationMakeColMapAndReindexSerial (const Teuchos::ArrayView<const size_t> &rowptr,
                                      const Teuchos::ArrayView<LocalOrdinal> &colind_LID,
                                      const Teuchos::ArrayView<GlobalOrdinal> &colind_GID,
                                      const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& domainMapRCP,
                                      const Teuchos::ArrayView<const int> &owningPIDs,
                                      Teuchos::Array<int> &remotePIDs,
                                      Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & colMap)
{
  using Teuchos::rcp;
  typedef LocalOrdinal LO;
  typedef GlobalOrdinal GO;
  typedef Tpetra::global_size_t GST;
  typedef Tpetra::Map<LO, GO, Node> map_type;
  const char prefix[] = "lowCommunicationMakeColMapAndReindexSerial: ";

  // The domainMap is an RCP because there is a shortcut for a
  // (common) special case to return the columnMap = domainMap.
  const map_type& domainMap = *domainMapRCP;

  // Scan all column indices and sort into two groups:
  // Local:  those whose GID matches a GID of the domain map on this processor and
  // Remote: All others.
  const size_t numDomainElements = domainMap.getLocalNumElements ();
  Teuchos::Array<bool> LocalGIDs;
  if (numDomainElements > 0) {
    LocalGIDs.resize (numDomainElements, false); // Assume domain GIDs are not local
  }

  // In principle it is good to have RemoteGIDs and RemotGIDList be as
  // long as the number of remote GIDs on this processor, but this
  // would require two passes through the column IDs, so we make it
  // the max of 100 and the number of block rows.
  //
  // FIXME (mfh 11 Feb 2015) Tpetra::Details::HashTable can hold at
  // most INT_MAX entries, but it's possible to have more rows than
  // that (if size_t is 64 bits and int is 32 bits).
  const size_t numMyRows = rowptr.size () - 1;
  const int hashsize = std::max (static_cast<int> (numMyRows), 100);

  Tpetra::Details::HashTable<GO, LO> RemoteGIDs (hashsize);
  Teuchos::Array<GO> RemoteGIDList;
  RemoteGIDList.reserve (hashsize);
  Teuchos::Array<int> PIDList;
  PIDList.reserve (hashsize);

  // Here we start using the *LocalOrdinal* colind_LID array.  This is
  // safe even if both columnIndices arrays are actually the same
  // (because LocalOrdinal==GO).  For *local* GID's set
  // colind_LID with with their LID in the domainMap.  For *remote*
  // GIDs, we set colind_LID with (numDomainElements+NumRemoteColGIDs)
  // before the increment of the remote count.  These numberings will
  // be separate because no local LID is greater than
  // numDomainElements.

  size_t NumLocalColGIDs = 0;
  LO NumRemoteColGIDs = 0;
  for (size_t i = 0; i < numMyRows; ++i) {
    for(size_t j = rowptr[i]; j < rowptr[i+1]; ++j) {
      const GO GID = colind_GID[j];
      // Check if GID matches a row GID
      const LO LID = domainMap.getLocalElement (GID);
      if(LID != -1) {
        const bool alreadyFound = LocalGIDs[LID];
        if (! alreadyFound) {
          LocalGIDs[LID] = true; // There is a column in the graph associated with this domain map GID
          NumLocalColGIDs++;
        }
        colind_LID[j] = LID;
      }
      else {
        const LO hash_value = RemoteGIDs.get (GID);
        if (hash_value == -1) { // This means its a new remote GID
          const int PID = owningPIDs[j];
          TEUCHOS_TEST_FOR_EXCEPTION(
            PID == -1, std::invalid_argument, prefix << "Cannot figure out if "
            "PID is owned.");
          colind_LID[j] = static_cast<LO> (numDomainElements + NumRemoteColGIDs);
          RemoteGIDs.add (GID, NumRemoteColGIDs);
          RemoteGIDList.push_back (GID);
          PIDList.push_back (PID);
          NumRemoteColGIDs++;
        }
        else {
          colind_LID[j] = static_cast<LO> (numDomainElements + hash_value);
        }
      }
    }
  }

  // Possible short-circuit: If all domain map GIDs are present as
  // column indices, then set ColMap=domainMap and quit.
  if (domainMap.getComm ()->getSize () == 1) {
    // Sanity check: When there is only one process, there can be no
    // remoteGIDs.
    TEUCHOS_TEST_FOR_EXCEPTION(
      NumRemoteColGIDs != 0, std::runtime_error, prefix << "There is only one "
      "process in the domain Map's communicator, which means that there are no "
      "\"remote\" indices.  Nevertheless, some column indices are not in the "
      "domain Map.");
    if (static_cast<size_t> (NumLocalColGIDs) == numDomainElements) {
      // In this case, we just use the domainMap's indices, which is,
      // not coincidently, what we clobbered colind with up above
      // anyway.  No further reindexing is needed.
      colMap = domainMapRCP;
      return;
    }
  }

  // Now build the array containing column GIDs
  // Build back end, containing remote GIDs, first
  const LO numMyCols = NumLocalColGIDs + NumRemoteColGIDs;
  Teuchos::Array<GO> ColIndices;
  GO* RemoteColIndices = NULL;
  if (numMyCols > 0) {
    ColIndices.resize (numMyCols);
    if (NumLocalColGIDs != static_cast<size_t> (numMyCols)) {
      RemoteColIndices = &ColIndices[NumLocalColGIDs]; // Points to back half of ColIndices
    }
  }

  for (LO i = 0; i < NumRemoteColGIDs; ++i) {
    RemoteColIndices[i] = RemoteGIDList[i];
  }

  // Build permute array for *remote* reindexing.
  Teuchos::Array<LO> RemotePermuteIDs (NumRemoteColGIDs);
  for (LO i = 0; i < NumRemoteColGIDs; ++i) {
    RemotePermuteIDs[i]=i;
  }

  // Sort External column indices so that all columns coming from a
  // given remote processor are contiguous.  This is a sort with two
  // auxilary arrays: RemoteColIndices and RemotePermuteIDs.
  Tpetra::sort3 (PIDList.begin (), PIDList.end (),
                 ColIndices.begin () + NumLocalColGIDs,
                 RemotePermuteIDs.begin ());

  // Stash the RemotePIDs.
  //
  // Note: If Teuchos::Array had a shrink_to_fit like std::vector,
  // we'd call it here.
  remotePIDs = PIDList;

  // Sort external column indices so that columns from a given remote
  // processor are not only contiguous but also in ascending
  // order. NOTE: I don't know if the number of externals associated
  // with a given remote processor is known at this point ... so I
  // count them here.

  // NTS: Only sort the RemoteColIndices this time...
  LO StartCurrent = 0, StartNext = 1;
  while (StartNext < NumRemoteColGIDs) {
    if (PIDList[StartNext]==PIDList[StartNext-1]) {
      StartNext++;
    }
    else {
      Tpetra::sort2 (ColIndices.begin () + NumLocalColGIDs + StartCurrent,
                     ColIndices.begin () + NumLocalColGIDs + StartNext,
                     RemotePermuteIDs.begin () + StartCurrent);
      StartCurrent = StartNext;
      StartNext++;
    }
  }
  Tpetra::sort2 (ColIndices.begin () + NumLocalColGIDs + StartCurrent,
                 ColIndices.begin () + NumLocalColGIDs + StartNext,
                 RemotePermuteIDs.begin () + StartCurrent);

  // Reverse the permutation to get the information we actually care about
  Teuchos::Array<LO> ReverseRemotePermuteIDs (NumRemoteColGIDs);
  for (LO i = 0; i < NumRemoteColGIDs; ++i) {
    ReverseRemotePermuteIDs[RemotePermuteIDs[i]] = i;
  }

  // Build permute array for *local* reindexing.
  bool use_local_permute = false;
  Teuchos::Array<LO> LocalPermuteIDs (numDomainElements);

  // Now fill front end. Two cases:
  //
  // (1) If the number of Local column GIDs is the same as the number
  //     of Local domain GIDs, we can simply read the domain GIDs into
  //     the front part of ColIndices, otherwise
  //
  // (2) We step through the GIDs of the domainMap, checking to see if
  //     each domain GID is a column GID.  we want to do this to
  //     maintain a consistent ordering of GIDs between the columns
  //     and the domain.
  Teuchos::ArrayView<const GO> domainGlobalElements = domainMap.getLocalElementList();
  if (static_cast<size_t> (NumLocalColGIDs) == numDomainElements) {
    if (NumLocalColGIDs > 0) {
      // Load Global Indices into first numMyCols elements column GID list
      std::copy (domainGlobalElements.begin (), domainGlobalElements.end (),
                 ColIndices.begin ());
    }
  }
  else {
    LO NumLocalAgain = 0;
    use_local_permute = true;
    for (size_t i = 0; i < numDomainElements; ++i) {
      if (LocalGIDs[i]) {
        LocalPermuteIDs[i] = NumLocalAgain;
        ColIndices[NumLocalAgain++] = domainGlobalElements[i];
      }
    }
    TEUCHOS_TEST_FOR_EXCEPTION(
      static_cast<size_t> (NumLocalAgain) != NumLocalColGIDs,
      std::runtime_error, prefix << "Local ID count test failed.");
  }

  // Make column Map
  const GST minus_one = Teuchos::OrdinalTraits<GST>::invalid ();
  colMap = rcp (new map_type (minus_one, ColIndices, domainMap.getIndexBase (),
                              domainMap.getComm ()));

  // Low-cost reindex of the matrix
  for (size_t i = 0; i < numMyRows; ++i) {
    for (size_t j = rowptr[i]; j < rowptr[i+1]; ++j) {
      const LO ID = colind_LID[j];
      if (static_cast<size_t> (ID) < numDomainElements) {
        if (use_local_permute) {
          colind_LID[j] = LocalPermuteIDs[colind_LID[j]];
        }
        // In the case where use_local_permute==false, we just copy
        // the DomainMap's ordering, which it so happens is what we
        // put in colind_LID to begin with.
      }
      else {
        colind_LID[j] =  NumLocalColGIDs + ReverseRemotePermuteIDs[colind_LID[j]-numDomainElements];
      }
    }
  }
}


template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void
lowCommunicationMakeColMapAndReindex (
                                      const Teuchos::ArrayView<const size_t> &rowptr,
                                      const Teuchos::ArrayView<LocalOrdinal> &colind_LID,
                                      const Teuchos::ArrayView<GlobalOrdinal> &colind_GID,
                                      const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& domainMapRCP,
                                      const Teuchos::ArrayView<const int> &owningPIDs,
                                      Teuchos::Array<int> &remotePIDs,
                                      Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & colMap)
{
  using Teuchos::rcp;
  typedef LocalOrdinal LO;
  typedef GlobalOrdinal GO;
  typedef Tpetra::global_size_t GST;
  typedef Tpetra::Map<LO, GO, Node> map_type;
  const char prefix[] = "lowCommunicationMakeColMapAndReindex: ";

  typedef typename Node::device_type DT;
  using execution_space = typename DT::execution_space;
  execution_space exec;
  using team_policy = Kokkos::TeamPolicy<execution_space, Kokkos::Schedule<Kokkos::Dynamic>>;
  typedef typename map_type::local_map_type local_map_type;

  // Create device mirror and host mirror views from function parameters
  // When we pass in views instead of Teuchos::ArrayViews, we can avoid copying views
  auto colind_LID_view = Details::create_mirror_view_from_raw_host_array(exec, colind_LID.getRawPtr(), colind_LID.size(), true, "colind_LID");
  auto rowptr_view = Details::create_mirror_view_from_raw_host_array(exec, rowptr.getRawPtr(), rowptr.size(), true, "rowptr");
  auto colind_GID_view = Details::create_mirror_view_from_raw_host_array(exec, colind_GID.getRawPtr(), colind_GID.size(), true, "colind_GID");
  auto owningPIDs_view = Details::create_mirror_view_from_raw_host_array(exec, owningPIDs.getRawPtr(), owningPIDs.size(), true, "owningPIDs");

  typename decltype(colind_LID_view)::HostMirror colind_LID_host(colind_LID.getRawPtr(), colind_LID.size());
  typename decltype(colind_GID_view)::HostMirror colind_GID_host(colind_GID.getRawPtr(), colind_GID.size());

  Kokkos::deep_copy(colind_LID_view, colind_LID_host);
  Kokkos::deep_copy(colind_GID_view, colind_GID_host);

  // The domainMap is an RCP because there is a shortcut for a
  // (common) special case to return the columnMap = domainMap.
  const map_type& domainMap = *domainMapRCP;

  Kokkos::UnorderedMap<LO, bool, DT> LocalGIDs_view_map(colind_LID.size());
  Kokkos::UnorderedMap<GO, LO, DT> RemoteGIDs_view_map(colind_LID.size());
  
  const size_t numMyRows = rowptr.size () - 1;
  local_map_type domainMap_local = domainMap.getLocalMap();
  
  const size_t numDomainElements = domainMap.getLocalNumElements ();
  Kokkos::View<bool*, DT> LocalGIDs_view("LocalGIDs", numDomainElements);
  auto LocalGIDs_host = Kokkos::create_mirror_view(LocalGIDs_view);
  
  size_t NumLocalColGIDs = 0;
  
  // Scan all column indices and sort into two groups:
  // Local:  those whose GID matches a GID of the domain map on this processor and
  // Remote: All others.
  // Kokkos::Parallel_reduce sums up NumLocalColGIDs, while we use the size of the Remote GIDs map to find NumRemoteColGIDs
  Kokkos::parallel_reduce(team_policy(numMyRows, Kokkos::AUTO), KOKKOS_LAMBDA(const typename team_policy::member_type &member, size_t &update) { 
    const int i = member.league_rank();
    size_t NumLocalColGIDs_temp = 0;
    size_t rowptr_start = rowptr_view[i];
    size_t rowptr_end = rowptr_view[i+1];
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(member, rowptr_start, rowptr_end), [&](const size_t j, size_t &innerUpdate) {
      const GO GID = colind_GID_view[j];
      // Check if GID matches a row GID in local domain map
      const LO LID = domainMap_local.getLocalElement (GID);
      if (LID != -1) {
        auto outcome = LocalGIDs_view_map.insert(LID);
        // Fresh insert
        if(outcome.success()) {
          LocalGIDs_view[LID] = true;
          innerUpdate++;
        }
      }
      else {
        const int PID = owningPIDs_view[j];
        auto outcome = RemoteGIDs_view_map.insert(GID, PID);
        if(outcome.success() && PID == -1) {
          Kokkos::abort("Cannot figure out if ID is owned.\n");
        }
      }
    }, NumLocalColGIDs_temp);
    if(member.team_rank() == 0) update += NumLocalColGIDs_temp;
  }, NumLocalColGIDs);
  
  LO NumRemoteColGIDs = RemoteGIDs_view_map.size();
  
  Kokkos::View<int*, DT> PIDList_view("PIDList", NumRemoteColGIDs);
  auto PIDList_host = Kokkos::create_mirror_view(PIDList_view);
  
  Kokkos::View<int*, DT> RemoteGIDList_view("RemoteGIDList", NumRemoteColGIDs);
  auto RemoteGIDList_host = Kokkos::create_mirror_view(RemoteGIDList_view);

  // For each index in RemoteGIDs_map that contains a GID, use "update" to indicate the number of GIDs "before" this GID
  // This maps each element in the RemoteGIDs hash table to an index in RemoteGIDList / PIDList without any overwriting or empty spaces between indices
  Kokkos::parallel_scan(Kokkos::RangePolicy<execution_space>(0, RemoteGIDs_view_map.capacity()), KOKKOS_LAMBDA(const int i, GO& update, const bool final) {
    if(final && RemoteGIDs_view_map.valid_at(i)) {
      RemoteGIDList_view[update] = RemoteGIDs_view_map.key_at(i);
      PIDList_view[update] = RemoteGIDs_view_map.value_at(i);
    }
    if(RemoteGIDs_view_map.valid_at(i)) {
      update += 1;
    }
  });

  // Possible short-circuit: If all domain map GIDs are present as
  // column indices, then set ColMap=domainMap and quit.
  if (domainMap.getComm ()->getSize () == 1) {
    // Sanity check: When there is only one process, there can be no
    // remoteGIDs.
    TEUCHOS_TEST_FOR_EXCEPTION(
      NumRemoteColGIDs != 0, std::runtime_error, prefix << "There is only one "
      "process in the domain Map's communicator, which means that there are no "
      "\"remote\" indices.  Nevertheless, some column indices are not in the "
      "domain Map.");
    if (static_cast<size_t> (NumLocalColGIDs) == numDomainElements) {
      // In this case, we just use the domainMap's indices, which is,
      // not coincidently, what we clobbered colind with up above
      // anyway.  No further reindexing is needed.
      colMap = domainMapRCP;
      
      // Fill out local colMap (which should only contain local GIDs)
      auto localColMap = colMap->getLocalMap();
      Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, colind_GID.size()), KOKKOS_LAMBDA(const int i) {
        colind_LID_view[i] = localColMap.getLocalElement(colind_GID_view[i]);
      });
      Kokkos::deep_copy(execution_space(), colind_LID_host, colind_LID_view);
      return;
    }
  }

  // Now build the array containing column GIDs
  // Build back end, containing remote GIDs, first
  const LO numMyCols = NumLocalColGIDs + NumRemoteColGIDs;
  Kokkos::View<GO*, DT> ColIndices_view("ColIndices", numMyCols);
  auto ColIndices_host = Kokkos::create_mirror_view(ColIndices_view);
  
  // We don't need to load the backend of ColIndices or sort if there are no remote GIDs
  if(NumRemoteColGIDs > 0) {
    if(NumLocalColGIDs != static_cast<size_t> (numMyCols)) {
      Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, NumRemoteColGIDs), KOKKOS_LAMBDA(const int i) {
        ColIndices_view[NumLocalColGIDs+i] = RemoteGIDList_view[i];
      });
    }  
  
    // Find the largest PID for bin sorting purposes
    int PID_max = 0;
    Kokkos::parallel_reduce(Kokkos::RangePolicy<execution_space>(0, PIDList_host.size()), KOKKOS_LAMBDA(const int i, int& max) {
      if(max < PIDList_view[i]) max = PIDList_view[i];
    }, Kokkos::Max<int>(PID_max));
  
    using KeyViewTypePID = decltype(PIDList_view);
    using BinSortOpPID = Kokkos::BinOp1D<KeyViewTypePID>;
  
    // Make a subview of ColIndices for remote GID sorting
    auto ColIndices_subview = Kokkos::subview(ColIndices_view, Kokkos::make_pair(NumLocalColGIDs, ColIndices_view.size()));
  
    // Make binOp with bins = PID_max + 1, min = 0, max = PID_max
    BinSortOpPID binOp2(PID_max+1, 0, PID_max);
    
    // Sort External column indices so that all columns coming from a
    // given remote processor are contiguous.  This is a sort with one
    // auxilary array: RemoteColIndices
    Kokkos::BinSort<KeyViewTypePID, BinSortOpPID> bin_sort2(PIDList_view, 0, PIDList_view.size(), binOp2, false);
    bin_sort2.create_permute_vector(exec);
    bin_sort2.sort(exec, PIDList_view);
    bin_sort2.sort(exec, ColIndices_subview);
  
    // Deep copy back from device to host
    Kokkos::deep_copy(execution_space(), PIDList_host, PIDList_view);
  
    // Stash the RemotePIDs. Once remotePIDs is changed to become a Kokkos view, we can remove this and copy directly.
    // Note: If Teuchos::Array had a shrink_to_fit like std::vector,
    // we'd call it here.
    
    Teuchos::Array<int> PIDList(NumRemoteColGIDs);
    for(LO i = 0; i < NumRemoteColGIDs; ++i) {
      PIDList[i] = PIDList_host[i];
    }
  
    remotePIDs = PIDList;
  
    // Sort external column indices so that columns from a given remote
    // processor are not only contiguous but also in ascending
    // order. NOTE: I don't know if the number of externals associated
    // with a given remote processor is known at this point ... so I
    // count them here.
    LO StartCurrent = 0, StartNext = 1;
    while(StartNext < NumRemoteColGIDs) {
      if(PIDList_host[StartNext] == PIDList_host[StartNext-1]) {
        StartNext++;
      }
      else {
        Kokkos::sort(ColIndices_view, NumLocalColGIDs + StartCurrent, NumLocalColGIDs + StartNext);
        StartCurrent = StartNext;
        StartNext++;
      }
    }
  
    Kokkos::sort(ColIndices_view, NumLocalColGIDs + StartCurrent, NumLocalColGIDs + StartNext);
  }


  // Build permute array for *local* reindexing.

  // Now fill front end. Two cases:
  //
  // (1) If the number of Local column GIDs is the same as the number
  //     of Local domain GIDs, we can simply read the domain GIDs into
  //     the front part of ColIndices, otherwise
  //
  // (2) We step through the GIDs of the domainMap, checking to see if
  //     each domain GID is a column GID.  we want to do this to
  //     maintain a consistent ordering of GIDs between the columns
  //     and the domain.
  Teuchos::ArrayView<const GO> domainGlobalElements = domainMap.getLocalElementList();
  auto domainGlobalElements_view = Details::create_mirror_view_from_raw_host_array(exec, domainGlobalElements.getRawPtr(), domainGlobalElements.size(), true, "domainGlobalElements");
  
  if (static_cast<size_t> (NumLocalColGIDs) == numDomainElements) {
    if (NumLocalColGIDs > 0) {
      // Load Global Indices into first numMyCols elements column GID list
      Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, domainGlobalElements.size()), KOKKOS_LAMBDA(const int i) {
        ColIndices_view[i] = domainGlobalElements_view[i];
      });
    }
  }
  else {
    // This part isn't actually tested in the unit tests
    LO NumLocalAgain = 0;
    Kokkos::parallel_scan(Kokkos::RangePolicy<execution_space>(0, numDomainElements), KOKKOS_LAMBDA(const int i, LO& update, const bool final) {
      if(final && LocalGIDs_view[i]) {
        ColIndices_view[update] = domainGlobalElements_view[i];
      }
      if(LocalGIDs_view[i]) {
        update++;
      }
    }, NumLocalAgain);
    
    
    TEUCHOS_TEST_FOR_EXCEPTION(
      static_cast<size_t> (NumLocalAgain) != NumLocalColGIDs,
      std::runtime_error, prefix << "Local ID count test failed.");
  }

  // Make column Map
  const GST minus_one = Teuchos::OrdinalTraits<GST>::invalid ();
  
  colMap = rcp (new map_type (minus_one, ColIndices_view, domainMap.getIndexBase (),
                              domainMap.getComm ()));

  // Fill out colind_LID using local map
  auto localColMap = colMap->getLocalMap();
  Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, colind_GID.size()), KOKKOS_LAMBDA(const int i) {
    colind_LID_view[i] = localColMap.getLocalElement(colind_GID_view[i]);
  });

  // For now, we copy back into colind_LID_host (which also overwrites the colind_LID Tuechos array)
  // When colind_LID becomes a Kokkos View we can delete this
  Kokkos::deep_copy(exec, colind_LID_host, colind_LID_view);        
}

template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void
lowCommunicationMakeColMapAndReindex (
                                      const Kokkos::View<size_t*,typename Node::device_type>        rowptr_view,
                                      const Kokkos::View<LocalOrdinal*,typename Node::device_type>  colind_LID_view,
                                      const Kokkos::View<GlobalOrdinal*,typename Node::device_type> colind_GID_view,
                                      const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& domainMapRCP,
                                      const Teuchos::ArrayView<const int> &owningPIDs,
                                      Teuchos::Array<int> &remotePIDs,
                                      Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & colMap)
{
  using Teuchos::rcp;
  typedef LocalOrdinal LO;
  typedef GlobalOrdinal GO;
  typedef Tpetra::global_size_t GST;
  typedef Tpetra::Map<LO, GO, Node> map_type;
  const char prefix[] = "lowCommunicationMakeColMapAndReindex: ";

  typedef typename Node::device_type DT;
  using execution_space = typename DT::execution_space;
  execution_space exec;
  using team_policy = Kokkos::TeamPolicy<execution_space, Kokkos::Schedule<Kokkos::Dynamic>>;
  typedef typename map_type::local_map_type local_map_type;

  // Create device mirror and host mirror views from function parameters
  // When we pass in views instead of Teuchos::ArrayViews, we can avoid copying views
  auto owningPIDs_view = Details::create_mirror_view_from_raw_host_array(exec, owningPIDs.getRawPtr(), owningPIDs.size(), true, "owningPIDs");

  // The domainMap is an RCP because there is a shortcut for a
  // (common) special case to return the columnMap = domainMap.
  const map_type& domainMap = *domainMapRCP;

  Kokkos::UnorderedMap<LO, bool, DT> LocalGIDs_view_map(colind_LID_view.size());
  Kokkos::UnorderedMap<GO, LO, DT> RemoteGIDs_view_map(colind_LID_view.size());
  
  const size_t numMyRows = rowptr_view.size () - 1;
  local_map_type domainMap_local = domainMap.getLocalMap();
  
  const size_t numDomainElements = domainMap.getLocalNumElements ();
  Kokkos::View<bool*, DT> LocalGIDs_view("LocalGIDs", numDomainElements);
  auto LocalGIDs_host = Kokkos::create_mirror_view(LocalGIDs_view);
  
  size_t NumLocalColGIDs = 0;
  
  // Scan all column indices and sort into two groups:
  // Local:  those whose GID matches a GID of the domain map on this processor and
  // Remote: All others.
  // Kokkos::Parallel_reduce sums up NumLocalColGIDs, while we use the size of the Remote GIDs map to find NumRemoteColGIDs
  Kokkos::parallel_reduce(team_policy(numMyRows, Kokkos::AUTO), KOKKOS_LAMBDA(const typename team_policy::member_type &member, size_t &update) { 
    const int i = member.league_rank();
    size_t NumLocalColGIDs_temp = 0;
    size_t rowptr_start = rowptr_view[i];
    size_t rowptr_end = rowptr_view[i+1];
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(member, rowptr_start, rowptr_end), [&](const size_t j, size_t &innerUpdate) {
      const GO GID = colind_GID_view[j];
      // Check if GID matches a row GID in local domain map
      const LO LID = domainMap_local.getLocalElement (GID);
      if (LID != -1) {
        auto outcome = LocalGIDs_view_map.insert(LID);
        // Fresh insert
        if(outcome.success()) {
          LocalGIDs_view[LID] = true;
          innerUpdate++;
        }
      }
      else {
        const int PID = owningPIDs_view[j];
        auto outcome = RemoteGIDs_view_map.insert(GID, PID);
        if(outcome.success() && PID == -1) {
          printf("Cannot figure out if ID is owned.\n");
          Kokkos::abort("Cannot figure out if ID is owned.\n");
        }
      }
    }, NumLocalColGIDs_temp);
    if(member.team_rank() == 0) update += NumLocalColGIDs_temp;
  }, NumLocalColGIDs);
  
  LO NumRemoteColGIDs = RemoteGIDs_view_map.size();
  
  Kokkos::View<int*, DT> PIDList_view("PIDList", NumRemoteColGIDs);
  
  Kokkos::View<int*, DT> RemoteGIDList_view("RemoteGIDList", NumRemoteColGIDs);
  auto RemoteGIDList_host = Kokkos::create_mirror_view(RemoteGIDList_view);

  // For each index in RemoteGIDs_map that contains a GID, use "update" to indicate the number of GIDs "before" this GID
  // This maps each element in the RemoteGIDs hash table to an index in RemoteGIDList / PIDList without any overwriting or empty spaces between indices
  Kokkos::parallel_scan(Kokkos::RangePolicy<execution_space>(0, RemoteGIDs_view_map.capacity()), KOKKOS_LAMBDA(const int i, GO& update, const bool final) {
    if(final && RemoteGIDs_view_map.valid_at(i)) {
      RemoteGIDList_view[update] = RemoteGIDs_view_map.key_at(i);
      PIDList_view[update] = RemoteGIDs_view_map.value_at(i);
    }
    if(RemoteGIDs_view_map.valid_at(i)) {
      update += 1;
    }
  });

  // Possible short-circuit: If all domain map GIDs are present as
  // column indices, then set ColMap=domainMap and quit.
  if (domainMap.getComm ()->getSize () == 1) {
    // Sanity check: When there is only one process, there can be no
    // remoteGIDs.
    TEUCHOS_TEST_FOR_EXCEPTION(
      NumRemoteColGIDs != 0, std::runtime_error, prefix << "There is only one "
      "process in the domain Map's communicator, which means that there are no "
      "\"remote\" indices.  Nevertheless, some column indices are not in the "
      "domain Map.");
    if (static_cast<size_t> (NumLocalColGIDs) == numDomainElements) {
      // In this case, we just use the domainMap's indices, which is,
      // not coincidently, what we clobbered colind with up above
      // anyway.  No further reindexing is needed.
      colMap = domainMapRCP;
      
      // Fill out local colMap (which should only contain local GIDs)
      auto localColMap = colMap->getLocalMap();
      Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, colind_GID_view.size()), KOKKOS_LAMBDA(const int i) {
        colind_LID_view[i] = localColMap.getLocalElement(colind_GID_view[i]);
      });
      return;
    }
  }

  // Now build the array containing column GIDs
  // Build back end, containing remote GIDs, first
  const LO numMyCols = NumLocalColGIDs + NumRemoteColGIDs;
  Kokkos::View<GO*, DT> ColIndices_view("ColIndices", numMyCols);
  auto ColIndices_host = Kokkos::create_mirror_view(ColIndices_view);
  
  // We don't need to load the backend of ColIndices or sort if there are no remote GIDs
  if(NumRemoteColGIDs > 0) {
    if(NumLocalColGIDs != static_cast<size_t> (numMyCols)) {
      Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, NumRemoteColGIDs), KOKKOS_LAMBDA(const int i) {
        ColIndices_view[NumLocalColGIDs+i] = RemoteGIDList_view[i];
      });
    }  
  
    // Find the largest PID for bin sorting purposes
    int PID_max = 0;
    Kokkos::parallel_reduce(Kokkos::RangePolicy<execution_space>(0, PIDList_view.size()), KOKKOS_LAMBDA(const int i, int& max) {
      if(max < PIDList_view[i]) max = PIDList_view[i];
    }, Kokkos::Max<int>(PID_max));
  
    using KeyViewTypePID = decltype(PIDList_view);
    using BinSortOpPID = Kokkos::BinOp1D<KeyViewTypePID>;
  
    // Make a subview of ColIndices for remote GID sorting
    auto ColIndices_subview = Kokkos::subview(ColIndices_view, Kokkos::make_pair(NumLocalColGIDs, ColIndices_view.size()));
  
    // Make binOp with bins = PID_max + 1, min = 0, max = PID_max
    BinSortOpPID binOp2(PID_max+1, 0, PID_max);
    
    // Sort External column indices so that all columns coming from a
    // given remote processor are contiguous.  This is a sort with one
    // auxilary array: RemoteColIndices
    Kokkos::BinSort<KeyViewTypePID, BinSortOpPID> bin_sort2(PIDList_view, 0, PIDList_view.size(), binOp2, false);
    bin_sort2.create_permute_vector(exec);
    bin_sort2.sort(exec, PIDList_view);
    bin_sort2.sort(exec, ColIndices_subview);
  
    // Deep copy back from device to host
    // Stash the RemotePIDs. Once remotePIDs is changed to become a Kokkos view, we can remove this and copy directly.
    Teuchos::Array<int> PIDList(NumRemoteColGIDs);
    Kokkos::View<int*, Kokkos::HostSpace> PIDList_host(PIDList.data(), PIDList.size());
    Kokkos::deep_copy(exec, PIDList_host, PIDList_view);
    exec.fence();
  
    remotePIDs = PIDList;
  
    // Sort external column indices so that columns from a given remote
    // processor are not only contiguous but also in ascending
    // order. NOTE: I don't know if the number of externals associated
    // with a given remote processor is known at this point ... so I
    // count them here.
    LO StartCurrent = 0, StartNext = 1;
    while(StartNext < NumRemoteColGIDs) {
      if(PIDList_host[StartNext] == PIDList_host[StartNext-1]) {
        StartNext++;
      }
      else {
        Kokkos::sort(ColIndices_view, NumLocalColGIDs + StartCurrent, NumLocalColGIDs + StartNext);
        StartCurrent = StartNext;
        StartNext++;
      }
    }
  
    Kokkos::sort(ColIndices_view, NumLocalColGIDs + StartCurrent, NumLocalColGIDs + StartNext);
  }


  // Build permute array for *local* reindexing.

  // Now fill front end. Two cases:
  //
  // (1) If the number of Local column GIDs is the same as the number
  //     of Local domain GIDs, we can simply read the domain GIDs into
  //     the front part of ColIndices, otherwise
  //
  // (2) We step through the GIDs of the domainMap, checking to see if
  //     each domain GID is a column GID.  we want to do this to
  //     maintain a consistent ordering of GIDs between the columns
  //     and the domain.
  Teuchos::ArrayView<const GO> domainGlobalElements = domainMap.getLocalElementList();
  auto domainGlobalElements_view = Details::create_mirror_view_from_raw_host_array(exec, domainGlobalElements.getRawPtr(), domainGlobalElements.size(), true, "domainGlobalElements");
  
  if (static_cast<size_t> (NumLocalColGIDs) == numDomainElements) {
    if (NumLocalColGIDs > 0) {
      // Load Global Indices into first numMyCols elements column GID list
      Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, domainGlobalElements.size()), KOKKOS_LAMBDA(const int i) {
        ColIndices_view[i] = domainGlobalElements_view[i];
      });
    }
  }
  else {
    // This part isn't actually tested in the unit tests
    LO NumLocalAgain = 0;
    Kokkos::parallel_scan(Kokkos::RangePolicy<execution_space>(0, numDomainElements), KOKKOS_LAMBDA(const int i, LO& update, const bool final) {
      if(final && LocalGIDs_view[i]) {
        ColIndices_view[update] = domainGlobalElements_view[i];
      }
      if(LocalGIDs_view[i]) {
        update++;
      }
    }, NumLocalAgain);
    
    
    TEUCHOS_TEST_FOR_EXCEPTION(
      static_cast<size_t> (NumLocalAgain) != NumLocalColGIDs,
      std::runtime_error, prefix << "Local ID count test failed.");
  }

  // Make column Map
  const GST minus_one = Teuchos::OrdinalTraits<GST>::invalid ();
  
  colMap = rcp (new map_type (minus_one, ColIndices_view, domainMap.getIndexBase (),
                              domainMap.getComm ()));

  // Fill out colind_LID using local map
  auto localColMap = colMap->getLocalMap();
  Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(0, colind_GID_view.size()), KOKKOS_LAMBDA(const int i) {
    colind_LID_view[i] = localColMap.getLocalElement(colind_GID_view[i]);
  });
}

// Generates an list of owning PIDs based on two transfer (aka import/export objects)
// Let:
//   OwningMap = useReverseModeForOwnership ? transferThatDefinesOwnership.getTargetMap() : transferThatDefinesOwnership.getSourceMap();
//   MapAo     = useReverseModeForOwnership ? transferThatDefinesOwnership.getSourceMap() : transferThatDefinesOwnership.getTargetMap();
//   MapAm     = useReverseModeForMigration ? transferThatDefinesMigration.getTargetMap() : transferThatDefinesMigration.getSourceMap();
//   VectorMap = useReverseModeForMigration ? transferThatDefinesMigration.getSourceMap() : transferThatDefinesMigration.getTargetMap();
// Precondition:
//  1) MapAo.isSameAs(*MapAm)                      - map compatibility between transfers
//  2) VectorMap->isSameAs(*owningPIDs->getMap())  - map compabibility between transfer & vector
//  3) OwningMap->isOneToOne()                     - owning map is 1-to-1
//  --- Precondition 3 is only checked in DEBUG mode ---
// Postcondition:
//   owningPIDs[VectorMap->getLocalElement(GID i)] =   j iff  (OwningMap->isLocalElement(GID i) on rank j)
template <typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void getTwoTransferOwnershipVector(const ::Tpetra::Details::Transfer<LocalOrdinal, GlobalOrdinal, Node>& transferThatDefinesOwnership,
                                   bool useReverseModeForOwnership,
                                   const ::Tpetra::Details::Transfer<LocalOrdinal, GlobalOrdinal, Node>& transferThatDefinesMigration,
                                   bool useReverseModeForMigration,
                                   Tpetra::Vector<int,LocalOrdinal,GlobalOrdinal,Node> & owningPIDs) {
  typedef Tpetra::Import<LocalOrdinal, GlobalOrdinal, Node> import_type;
  typedef Tpetra::Export<LocalOrdinal, GlobalOrdinal, Node> export_type;

  Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > OwningMap = useReverseModeForOwnership ?
                                                                                transferThatDefinesOwnership.getTargetMap() :
                                                                                transferThatDefinesOwnership.getSourceMap();
  Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > MapAo     = useReverseModeForOwnership ?
                                                                                transferThatDefinesOwnership.getSourceMap() :
                                                                                transferThatDefinesOwnership.getTargetMap();
  Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > MapAm     = useReverseModeForMigration ?
                                                                                transferThatDefinesMigration.getTargetMap() :
                                                                                transferThatDefinesMigration.getSourceMap();
  Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > VectorMap = useReverseModeForMigration ?
                                                                                transferThatDefinesMigration.getSourceMap() :
                                                                                transferThatDefinesMigration.getTargetMap();

  TEUCHOS_TEST_FOR_EXCEPTION(!MapAo->isSameAs(*MapAm),std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector map mismatch between transfers");
  TEUCHOS_TEST_FOR_EXCEPTION(!VectorMap->isSameAs(*owningPIDs.getMap()),std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector map mismatch transfer and vector");
#ifdef HAVE_TPETRA_DEBUG
  TEUCHOS_TEST_FOR_EXCEPTION(!OwningMap->isOneToOne(),std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector owner must be 1-to-1");
#endif

  int rank = OwningMap->getComm()->getRank();
  // Generate "A" vector and fill it with owning information.  We can read this from transferThatDefinesOwnership w/o communication
  // Note:  Due to the 1-to-1 requirement, several of these options throw
  Tpetra::Vector<int,LocalOrdinal,GlobalOrdinal,Node> temp(MapAo);
  const import_type* ownAsImport = dynamic_cast<const import_type*> (&transferThatDefinesOwnership);
  const export_type* ownAsExport = dynamic_cast<const export_type*> (&transferThatDefinesOwnership);

  Teuchos::ArrayRCP<int> pids    = temp.getDataNonConst();
  Teuchos::ArrayView<int> v_pids = pids();
  if(ownAsImport && useReverseModeForOwnership)       {TEUCHOS_TEST_FOR_EXCEPTION(1,std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector owner must be 1-to-1");}
  else if(ownAsImport && !useReverseModeForOwnership) getPids(*ownAsImport,v_pids,false);
  else if(ownAsExport && useReverseModeForMigration)  {TEUCHOS_TEST_FOR_EXCEPTION(1,std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector this option not yet implemented");}
  else                                                {TEUCHOS_TEST_FOR_EXCEPTION(1,std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector owner must be 1-to-1");}

  const import_type* xferAsImport = dynamic_cast<const import_type*> (&transferThatDefinesMigration);
  const export_type* xferAsExport = dynamic_cast<const export_type*> (&transferThatDefinesMigration);
  TEUCHOS_TEST_FOR_EXCEPTION(!xferAsImport && !xferAsExport,std::runtime_error,"Tpetra::Import_Util::getTwoTransferOwnershipVector transfer undefined");

  // Migrate from "A" vector to output vector
  owningPIDs.putScalar(rank);
  if(xferAsImport && useReverseModeForMigration)        owningPIDs.doExport(temp,*xferAsImport,Tpetra::REPLACE);
  else if(xferAsImport && !useReverseModeForMigration)  owningPIDs.doImport(temp,*xferAsImport,Tpetra::REPLACE);
  else if(xferAsExport && useReverseModeForMigration)   owningPIDs.doImport(temp,*xferAsExport,Tpetra::REPLACE);
  else                                                  owningPIDs.doExport(temp,*xferAsExport,Tpetra::REPLACE);

}



} // namespace Import_Util
} // namespace Tpetra

#endif // TPETRA_IMPORT_UTIL_HPP
