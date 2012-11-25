#include "itkImageCommon.h"
#include "itkCastImageFilter.h"
#include "itkPropagateBCFilter.h"
#include "itkLaplaceImageFilterRestrictedDomain.h"
#include "itkZeroImageFilter.h"
#include "itkExceptionObject.h"
#include "mainwindow.h"
#include <QtGui>
#include <QApplication>
#include <iostream>
#include <exception>

typedef itk::CastImageFilter<LabelType,ImageType> CastFilterType;
typedef itk::PropagateBCFilter<LabelType> PropagateBCFilterType;
typedef itk::LaplaceImageFilterRestrictedDomain<ImageType,LabelType,ImageType> LaplacePDEFilterType;
typedef itk::ZeroImageFilter<LabelType,ImageType> ZeroImageFilterType;

using namespace std;

class MainApps: public QApplication {
public:
    MainApps(int &argc, char* argv[]):
    QApplication(argc, argv) {
    }

    virtual ~MainApps() {
    }

    virtual bool notify(QObject* rec, QEvent* ev) {
        try {
            return QApplication::notify(rec, ev);
        } catch (logic_error& e) {
            cout << e.what() << endl;
        } catch (itk::ExceptionObject& i) {
            cout << i;
        }
        return true;
    }
};


static vnl_vector<int>& modifyVector(vnl_vector<int>& a) {
    vnl_vector<int> b = a;
    cout << b << endl;
    b[0] = 2;
    cout << b << endl;
    return a;
}

static void runTestCode() {
    vnl_vector<int> a(1);
    a[0] = 3;
    cout << a << endl;
    vnl_vector<int>& c = modifyVector(a);
    cout << a << endl;
    c[0] = 4;
    cout << a << endl;


}

static void runQStringSplit() {
    QRegExp sep("(\\s+|, )");
    QStringList selectedPoints = QString("1,2,3,4,5").split(sep, QString::SkipEmptyParts);
    cout << "splitList:" << selectedPoints.size() << endl;
    for (int i = 0; i < selectedPoints.size(); i++) {
        cout << "[" << selectedPoints[i].toStdString() << "]" << endl;
    }
}

/**
 * Compute laplacian field from a given boundary condition map
 *
 * Input (short type): boundary condition label map (1: source, 2: solution, 3: destination, 4: neumann boundary)
 * Output (double type): laplacian field range from 0 to 1
 */
int main(int argc, char* argv[]) {
  if (argc < 3) {
      runQStringSplit();

      MainApps apps(argc, argv);
      MainWindow w;
      w.ReadyToExperiments();
      w.show();
      return apps.exec();
  }
  int err = 0;
  LabelType::Pointer inputLabel = ReadImageT<LabelType>(argv[1], err);

  PropagateBCFilterType::Pointer bcFilter = PropagateBCFilterType::New();
  bcFilter->SetInput(inputLabel);
  bcFilter->SetDirichletLowId(1);
  bcFilter->SetDirichletHighId(3);
  bcFilter->SetNeumannId(4);
  bcFilter->SetSolutionDomainId(2);
  bcFilter->SetInput(inputLabel);
  bcFilter->Update();
  LabelType::Pointer bcMap = bcFilter->GetOutput();

  ZeroImageFilterType::Pointer zeroFilter = ZeroImageFilterType::New();
  zeroFilter->SetInput(inputLabel);
  zeroFilter->Update();
  ImageType::Pointer zeroImg = zeroFilter->GetOutput();

  LaplacePDEFilterType::Pointer lpFilter = LaplacePDEFilterType::New();
  lpFilter->SetInput(zeroImg);
  lpFilter->SetLabelImage(bcMap);
  lpFilter->SetOriginId(1);
  lpFilter->SetTargetId(3);
  lpFilter->SetSolutionDomainId(2);
  lpFilter->UseImageSpacingOn();
  lpFilter->SetTerminalNumberOfIterations(2000);
  lpFilter->Update();
  ImageType::Pointer laplaceImg = lpFilter->GetOutput();

  WriteImageT<ImageType>(argv[2], laplaceImg);
  return 0;
}
