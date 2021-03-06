/// \ingroup base
/// \class ttk::TrackingFromPersistenceDiagrams
/// \author Maxime Soler <soler.maxime@total.com>
/// \date August 2018.

#pragma once

// base code includes
#include <BottleneckDistance.h>
#include <PersistenceDiagram.h>
#include <Triangulation.h>
using namespace std;

namespace ttk {

  class TrackingFromFields : virtual public Debug {

  public:
    TrackingFromFields() {
      this->setDebugMsgPrefix("TrackingFromFields");
    }

    /// Execute the package.
    /// \return Returns 0 upon success, negative values otherwise.
    // template <class dataType>
    // int execute();

    template <typename dataType,
              typename triangulationType = ttk::AbstractTriangulation>
    int performDiagramComputation(
      int fieldNumber,
      std::vector<std::vector<diagramTuple>> &persistenceDiagrams,
      const triangulationType *triangulation);

    /// Pass a pointer to an input array representing a scalarfield.
    /// The array is expected to be correctly allocated. idx in
    /// [0,numberOfInputs_[ \param idx Index of the input scalar field. \param
    /// data Pointer to the data array. \return Returns 0 upon success, negative
    /// values otherwise. \sa setNumberOfInputs() and setVertexNumber().
    inline int setInputDataPointer(int idx, void *data) {
      if(idx < numberOfInputs_)
        inputData_[idx] = data;
      else
        return -1;
      return 0;
    }

    /// Set the number of input scalar fields
    /// \param numberOfInputs Number of input scalar fields.
    /// \return Returns 0 upon success, negative values otherwise
    inline int setNumberOfInputs(int numberOfInputs) {
      numberOfInputs_ = numberOfInputs;
      return 0;
    }

    inline void
      preconditionTriangulation(AbstractTriangulation *triangulation) {
      ttk::PersistenceDiagram pd{};
      pd.preconditionTriangulation(triangulation);
    }

    inline void setInputScalars(std::vector<void *> &is) {
      inputData_ = is;
    }

    inline void setInputOffsets(void *io) {
      inputOffsets_ = io;
    }

  protected:
    int numberOfInputs_{0};
    std::vector<void *> inputData_{};
    void *inputOffsets_{};
  };
} // namespace ttk

template <typename dataType, typename triangulationType>
int ttk::TrackingFromFields::performDiagramComputation(
  int fieldNumber,
  std::vector<std::vector<diagramTuple>> &persistenceDiagrams,
  const triangulationType *triangulation) {

#ifdef TTK_ENABLE_OPENMP
#pragma omp parallel for num_threads(threadNumber_)
#endif // TTK_ENABLE_OPENMP
  for(int i = 0; i < fieldNumber; ++i) {
    ttk::PersistenceDiagram persistenceDiagram;
    persistenceDiagram.setThreadNumber(1);

    // std::vector<std::tuple<ttk::dcg::Cell, ttk::dcg::Cell>> dmt_pairs;
    // persistenceDiagram.setDMTPairs(&dmt_pairs);
    // persistenceDiagram.setInputScalars(inputData_[i]);
    // persistenceDiagram.setInputOffsets(inputOffsets_);
    persistenceDiagram.setComputeSaddleConnectors(false);
    std::vector<std::tuple<int, CriticalType, int, CriticalType, dataType, int>>
      CTDiagram;

    // persistenceDiagram.setOutputCTDiagram(&CTDiagram);
    persistenceDiagram.execute<dataType, int, triangulationType>(
      CTDiagram, (dataType *)(inputData_[i]), (int *)(inputOffsets_),
      triangulation);

    // Copy diagram into augmented diagram.
    persistenceDiagrams[i] = std::vector<diagramTuple>(CTDiagram.size());

    for(int j = 0; j < (int)CTDiagram.size(); ++j) {
      float p[3];
      float q[3];
      auto currentTuple = CTDiagram[j];
      const int a = std::get<0>(currentTuple);
      const int b = std::get<2>(currentTuple);
      triangulation->getVertexPoint(a, p[0], p[1], p[2]);
      triangulation->getVertexPoint(b, q[0], q[1], q[2]);
      const double sa = ((double *)inputData_[i])[a];
      const double sb = ((double *)inputData_[i])[b];
      diagramTuple dt
        = std::make_tuple(std::get<0>(currentTuple), std::get<1>(currentTuple),
                          std::get<2>(currentTuple), std::get<3>(currentTuple),
                          std::get<4>(currentTuple), std::get<5>(currentTuple),
                          sa, p[0], p[1], p[2], sb, q[0], q[1], q[2]);

      persistenceDiagrams[i][j] = dt;
    }
  }

  return 0;
}
