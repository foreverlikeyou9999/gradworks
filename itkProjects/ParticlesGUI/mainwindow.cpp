//
//  mainwindow.cpp
//  laplacePDE
//
//  Created by Joohwi Lee on 11/13/12.
//
//

#include "mainwindow.h"
#include "QFileDialog"
#include "vtkRenderer.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "QVTKInteractor.h"
#include "myParticleAlgorithm.h"
#include <vtkBoxMuellerRandomSequence.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include "myImageParticlesAlgorithm.h"
#include "itkARGBColorFunction.h"

static int g_AnimFrame = 0;
static bool g_showTraceParticles = false;

surface::ParametersVectorType g_Params;
ImageParticlesAlgorithm::Pointer g_imageParticlesAlgo;
surface::ParticleAlgorithm::Pointer g_ParticleAlgo;


MainWindow::MainWindow(QWidget* parent): m_ParticleColors(this), m_Props(this) {
    ui.setupUi(this);

    ui.toolBar->addWidget(ui.zoomLabel);
    ui.toolBar->addWidget(ui.zoomRatio);
    
    m_Renderer = vtkRenderer::New();
    m_PropScene.SetRenderer(m_Renderer);

    vtkGenericOpenGLRenderWindow* renWin = ui.qvtkwidget->GetRenderWindow();
    renWin->AddRenderer(m_Renderer);
    m_Interactor = ui.qvtkwidget->GetInteractor();

    ui.graphicsView->setScene(&m_scene);
    ui.graphicsView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));


    QObject::connect(&m_Timer, SIGNAL(timeout()), this, SLOT(on_animationTimeout()));
    QObject::connect(ui.sliceIndex, SIGNAL(sliderMoved(int)), this, SLOT(updateScene()));
    QObject::connect(ui.showXY, SIGNAL(toggled(bool)), this, SLOT(chooseSlice()));
    QObject::connect(ui.showYZ, SIGNAL(toggled(bool)), this, SLOT(chooseSlice()));
    QObject::connect(ui.showZX, SIGNAL(toggled(bool)), this, SLOT(chooseSlice()));
    QObject::connect(ui.grayImages, SIGNAL(currentIndexChanged(int)), this, SLOT(selectImage(int)));
    QObject::connect(ui.labelImages, SIGNAL(currentIndexChanged(int)), this, SLOT(selectLabel(int)));

    QObject::connect(ui.zoomRatio, SIGNAL(valueChanged(double)), this, SLOT(updateScene()));
    QObject::connect(ui.labelOpacity, SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
    QObject::connect(&m_ParticleColors, SIGNAL(triggered(QAction*)), this, SLOT(updateScene()));
    QObject::connect(ui.actionShowParticles, SIGNAL(triggered()), this, SLOT(updateScene()));
    QObject::connect(ui.showGray, SIGNAL(toggled(bool)), this, SLOT(updateScene()));
    QObject::connect(ui.showLabel, SIGNAL(toggled(bool)), this, SLOT(updateScene()));
    QObject::connect(ui.showDerived, SIGNAL(toggled(bool)), this, SLOT(updateScene()));
    QObject::connect(ui.particlesInitialization, SIGNAL(clicked()), this, SLOT(on_actionRandomParticlesInit_triggered()));
    
	ui.graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    m_ParticleColors.addAction(ui.actionParticleBlack);
    m_ParticleColors.addAction(ui.actionParticleWhite);
    m_ParticleColors.addAction(ui.actionParticleRed);
    m_ParticleColors.addAction(ui.actionParticleGreen);
    m_ParticleColors.addAction(ui.actionParticleBlue);
    m_ParticleColors.addAction(ui.actionParticleHSV);

    ui.costPlot->setColor(QColor(0xf5, 0xf3, 0xff));
    ui.costPlot->addGraph();
}

MainWindow::~MainWindow() {

}

void MainWindow::on_derivedImages_currentIndexChanged(int n) {
    if (n > 0) {
        ui.showDerived->setCheckState(Qt::Checked);
    } else {
        ui.showDerived->setCheckState(Qt::Unchecked);
    }
    updateScene();
}

void MainWindow::EventRaised(int eventId, int eventCode, const void* src, void* data) {
    if (eventId == 0xADDCE) {
        ui.derivedImages->clear();
        ui.derivedImages->addItem("-----");
        ImageContainer::StringList derivedNames;
        ImageContainer::GetDerivedViewNames(derivedNames);
        for (int i = 0; i < derivedNames.size(); i++) {
            ui.derivedImages->addItem(derivedNames[i].c_str());
        }
    } else if (eventId == 0xADDCEC) {
        double* doubleData = (double*) data;
        ui.costPlot->graph()->addData(doubleData[0], doubleData[1]);
        ui.costPlot->rescaleAxes();
        ui.costPlot->xAxis->setLabel("iteration");
        ui.costPlot->yAxis->setLabel("cost");
        ui.costPlot->replot();
    }
}

void MainWindow::ReadyToExperiments() {
    LoadImage("/data/Particles/00.T2.nrrd");
    LoadLabel("/data/Particles/00.Label.nrrd");
//    LoadImage("/data/Particles/16.T2.nrrd");
//    LoadLabel("/data/Particles/16.Label.nrrd");
    LoadSurface("/data/Particles/00.vtk");
    on_actionRandomParticlesInit_triggered();
}

void MainWindow::selectImage(int idx) {
    updateScene();
}

void MainWindow::selectLabel(int idx) {
    updateScene();
}

void MainWindow::LoadImage(QString fileName) {
    ImageContainer::Pointer image;
    for (int i = 0; i < (int) m_ImageList.size(); i++) {
        if (!m_ImageList[i]->HasImage()) {
            image = m_ImageList[i];
        }
    }
    if (image.IsNull()) {
        image = ImageContainer::New();
        image->SetEventCallback(this);
        m_ImageList.push_back(image);
    }
    image->LoadImage(fileName.toUtf8().data());
    if (!image->HasLabel()) {
        ui.sliceIndex->setMaximum(image->GetSize()[0]);
        ui.sliceIndex->setValue(image->GetSliceIndex()[0]);
    }
    updateScene();
    ui.tabWidget->setCurrentWidget(ui.imageTab);
    ui.toolBox->setCurrentWidget(ui.imageSettings);
    ui.grayImages->addItem(fileName);
    g_imageParticlesAlgo = ImageParticlesAlgorithm::Pointer();
}

/**
 * Load label image and add to ImageList
 * Handles label selection combobox
 */
void MainWindow::LoadLabel(QString fileName) {
    ImageContainer::Pointer image;
    for (int i = 0; i < (int) m_ImageList.size(); i++) {
        if (!m_ImageList[i]->HasLabel()) {
            image = m_ImageList[i];
            break;
        }
    }
    if (image.IsNull()) {
        image = ImageContainer::New();
        image->SetEventCallback(this);
        m_ImageList.push_back(image);
    }

    image->LoadLabel(fileName.toUtf8().data());
    if (!image->HasImage()) {
        ui.sliceIndex->setMaximum(image->GetSize()[0]);
        ui.sliceIndex->setValue(image->GetSliceIndex()[0]);
    }
    updateScene();
    ui.tabWidget->setCurrentWidget(ui.imageTab);
    ui.toolBox->setCurrentWidget(ui.imageSettings);
    ui.labelImages->addItem(fileName);
    g_imageParticlesAlgo = ImageParticlesAlgorithm::Pointer();    
}

void MainWindow::LoadSurface(QString fileName) {
    vtkPolyData* poly = m_PropScene.LoadPolyData(fileName.toUtf8().data());
    m_PropScene.AddPolyData("mainSurface", poly);
    m_PropScene.SetColor(1, 0, 0);
    m_PropScene.SetRepresentation(0);

    m_Renderer->ResetCamera();
    m_Interactor->Render();
    

}
void MainWindow::on_actionAddImage_triggered() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/tmpfs/data", tr("Volumes (*.nrrd *.nii *.gipl.gz)"));
    if (fileName.isNull()) {
        return;
    }
    LoadImage(fileName);
}

void MainWindow::on_actionAddLabel_triggered() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/tmpfs/data", tr("Volumes (*.nrrd *.nii *.gipl.gz)"));
    if (fileName.isNull()) {
        return;
    }
    LoadLabel(fileName);
}


void MainWindow::on_actionOpenSurface_triggered() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/tmpfs/data", tr("Volumes (*.vtk)"));
    if (fileName.isNull()) {
        return;
    }

    LoadSurface(fileName);
//    ParticleAlgorithm::Pointer algo = ParticleAlgorithm::New();
//    algo->SetPropertyAccess(m_Props);
//    algo->SetInitialParticles(poly);
//    algo->RunOptimization();
//    g_Params = algo->GetParameterTrace();
    ////
    //    vtkPolyData* poly2 = poly->NewInstance();
    //    poly2->DeepCopy(poly);
    //
    //    vtkPointSet* result = algo->GetResultPoints();
    //    poly2->SetPoints(result->GetPoints());
}

void MainWindow::on_actionSurfaceSmoothing_triggered() {
    // surface smoothing assumes the first image and label is matched with the current surface model
    // apply the surface entropy minimization smoothing with labelmap restriction
    g_ParticleAlgo = surface::ParticleAlgorithm::New();
    g_ParticleAlgo->SetPropertyAccess(m_Props);
    g_ParticleAlgo->SetImageList(&m_ImageList);
    g_ParticleAlgo->SetModelScene(&m_PropScene);
    g_ParticleAlgo->RunOptimization();
}

void MainWindow::on_actionSurfaceSmoothingContinue_triggered() {
    if (g_ParticleAlgo.IsNull()) {
        return;
    }
}

void MainWindow::on_actionTest_triggered() {
    if (ui.tabWidget->currentWidget() == ui.modelTab) {
        vtkPolyData* object = m_PropScene.CreatePlane(10, 10, 0, 0);
        object->Print(cout);
        m_PropScene.AddPolyData("object", object);
        m_PropScene.SetColor(0, 0, 1);
        m_PropScene.SetRepresentation(1);


        m_Renderer->ResetCamera();
        m_Interactor->Render();
    }
}

void MainWindow::chooseSlice() {
    int sliceView = 0;
    if (ui.showXY->isChecked()) {
        sliceView = 0;
    } else if (ui.showYZ->isChecked()) {
        sliceView = 1;
    } else if (ui.showZX->isChecked()) {
        sliceView = 2;
    }
    for (int i = 0; i < m_ImageList.size(); i++) {
        m_ImageList[i]->SetSliceDir(sliceView);
    }
    updateScene();
    g_imageParticlesAlgo = ImageParticlesAlgorithm::Pointer(NULL);
}

void MainWindow::updateScene() {
    m_scene.clear();


    QTransform viewTransform;
    viewTransform.scale(m_Props.GetDouble("zoomRatio", 1), m_Props.GetDouble("zoomRatio", 1));
    ui.graphicsView->setTransform(viewTransform);

    int dim = GetCurrentView();
    int image = GetCurrentImage();
    int labelIdx = ui.labelImages->currentIndex();

    // Gray image rendering
    if (ui.derivedImages->currentIndex() > 0 && ui.showDerived->isChecked()) {
        m_scene.addPixmap(ImageContainer::GetDerivedViewPixmap(ui.derivedImages->currentText().toUtf8().data()));
    } else {
        if (m_ImageList.size() > image && ui.showGray->isChecked()) {
            ui.sliceIndex->setMaximum(m_ImageList[image]->GetSize()[dim]);
            m_ImageList[image]->SetSliceIndex(dim, ui.sliceIndex->value());
            QGraphicsPixmapItem* item = m_scene.addPixmap(m_ImageList[image]->GetPixmap(dim));
            ui.graphicsView->centerOn((const QGraphicsItem*) item);
        }

    }

    // Label image rendering
    if (m_ImageList.size() > labelIdx && ui.showLabel->isChecked()) {
        m_ImageList[labelIdx]->SetLabelAlpha(m_Props.GetInt("labelOpacity", 128));
        m_scene.addPixmap(m_ImageList[labelIdx]->GetLabelPixmap(dim));
    }

    // Particles rendering
    if (g_imageParticlesAlgo.IsNotNull() && ui.actionShowParticles->isChecked()) {
        if (g_imageParticlesAlgo->IsMarkerValid(dim, ui.sliceIndex->value())) {
            const OptimizerParametersType* particles = NULL;
            if (g_showTraceParticles) {
                particles = g_imageParticlesAlgo->GetTraceParameters(g_AnimFrame);
            } else {
                particles = &(g_imageParticlesAlgo->GetCurrentParams());
            }
            if (particles != NULL && particles->GetSize() >= (image+1)*g_imageParticlesAlgo->GetNumberOfParams()) {
                //            const int nSubj = g_imageParticlesAlgo->GetNumberOfSubjects();
                const int nPoints = g_imageParticlesAlgo->GetNumberOfPoints();
                const int nParams = g_imageParticlesAlgo->GetNumberOfParams();
                const int nDims = 2;
                
                itk::Function::HSVColormapFunction<double, itk::RGBAPixel<unsigned char> >::Pointer colorFunc = itk::Function::HSVColormapFunction<double, itk::RGBAPixel<unsigned char> >::New();
                colorFunc->SetMinimumInputValue(0);
                colorFunc->SetMaximumInputValue(nPoints);
                
                for (int i = 0; i < nPoints; i++) {
                    double x = particles->GetElement(image * nParams + nDims * i);
                    double y = particles->GetElement(image * nParams + nDims * i + 1);
                    
                    QColor pointColor = Qt::black;
                    if (ui.actionParticleRed->isChecked()) {
                        pointColor = Qt::red;
                    } else if (ui.actionParticleGreen->isChecked()) {
                        pointColor = Qt::green;
                    } else if (ui.actionParticleBlue->isChecked()) {
                        pointColor = Qt::blue;
                    } else if (ui.actionParticleWhite->isChecked()) {
                        pointColor = Qt::white;
                    } else if (ui.actionParticleHSV->isChecked()) {
                        itk::RGBAPixel<unsigned char> rgb = colorFunc->operator()(i);
                        pointColor = QColor(rgb[0], rgb[1], rgb[2]);
                    }
                    
                    m_scene.addEllipse(x, y, 1, 1, QPen(pointColor), QBrush(pointColor, Qt::SolidPattern));
                }
            }
        }
    }
    
//    QRectF sceneRect = ui.graphicsView->sceneRect();
//    cout << "Rect [" << sceneRect.top() << "," << sceneRect.bottom() << "," << sceneRect.left() << "," << sceneRect.right() << endl;
}


void MainWindow::on_actionAnimation_triggered() {
    g_AnimFrame = 0;
    m_Timer.start(m_Props.GetInt("animationTimeout", 100));
}

void MainWindow::on_animationTimeout() {
    if (ui.tabWidget->currentWidget() == ui.imageTab) {
        // animation should play image particels if imageTab is showing
        g_showTraceParticles = true;
        if (g_imageParticlesAlgo.IsNotNull() && g_AnimFrame >= g_imageParticlesAlgo->GetNumberOfTraces()) {
            g_showTraceParticles = false;
            m_Timer.stop();
            return;
        }
        // the particle movement is handeled in updateScene() with g_showTraceParticles flag
        // this is not an optimal implementation because this will update every images together
        updateScene();
        ui.statusbar->showMessage(QString("%1 frame played...").arg(g_AnimFrame));        
        g_AnimFrame += m_Props.GetInt("animationInterleave", 10);
    } else if (ui.tabWidget->currentWidget() == ui.modelTab) {
        int imageIdx = ui.grayImages->currentIndex();
        if (imageIdx < 0) {
            m_Timer.stop();
            return;
        }

        vtkPolyData* poly = m_PropScene.FindPolyData("plane");
        if (poly == NULL || g_imageParticlesAlgo.IsNull()) {
            m_Timer.stop();
            return;
        }

//        if (g_AnimFrame < (int) g_Params.size()) {
//            OptimizerParametersType param = g_Params[g_AnimFrame];
//            if (param.GetSize() != 3 * poly->GetNumberOfPoints()) {
//                m_Timer.stop();
//                return;
//            }
        if (g_AnimFrame < (int) g_imageParticlesAlgo->GetNumberOfTraces()) {
            const OptimizerParametersType *param = g_imageParticlesAlgo->GetTraceParameters(g_AnimFrame);
            int nPoints = poly->GetNumberOfPoints();
            int nOffset = imageIdx * (nPoints * 2);
            for (int i = 0; i < nPoints; i++) {
                poly->GetPoints()->SetPoint(i, param->operator[](nOffset+2*i), param->operator[](nOffset+2*i+1), 0);
            }
            m_PropScene.ModifyLastActor();
            ui.statusbar->showMessage(QString("%1 frame showing").arg(g_AnimFrame));
            g_AnimFrame += m_Props.GetInt("animationInterleave", 10);
            m_Renderer->ResetCamera();
            m_Interactor->Render();
        }
    }
    
}

void MainWindow::on_actionRandomParticlesInit_triggered() {
    ui.toolBox->setCurrentWidget(ui.optimizerSettings);
    g_imageParticlesAlgo = ImageParticlesAlgorithm::New();
    g_imageParticlesAlgo->SetPropertyAccess(m_Props);
    g_imageParticlesAlgo->SetViewingDimension(GetCurrentView());
    g_imageParticlesAlgo->SetImageList(&m_ImageList);
    g_imageParticlesAlgo->SetEventCallback(this);
    g_imageParticlesAlgo->CreateRandomInitialPoints(m_Props.GetInt("numberOfPoints", 100));
    g_imageParticlesAlgo->SetSliceMarker(ui.sliceIndex->value());
    updateScene();
}


void MainWindow::on_actionRunImageParticles_triggered() {
    ui.toolBox->setCurrentWidget(ui.optimizerSettings);
    ui.costPlot->graph()->clearData();
    if (IsImageAvailable(0)) {
        if (g_imageParticlesAlgo.IsNotNull()) {
            ImageContainer::ClearDerivedViews();
            g_imageParticlesAlgo->SetImageList(&m_ImageList);
            // g_imageParticlesAlgo->SetPropertyAccess(m_Props);
            g_imageParticlesAlgo->RunOptimization();
        }
    }
    updateScene();
}

void MainWindow::on_actionContinue_triggered() {
    ui.toolBox->setCurrentWidget(ui.optimizerSettings);
    if (g_imageParticlesAlgo.IsNotNull()) {
        g_imageParticlesAlgo->ContinueOptimization();
        updateScene();
    }
}

void MainWindow::on_graphicsView_mousePressed(QMouseEvent* event) {
    QPoint o = event->pos();
    QPointF p = ui.graphicsView->mapToScene(o);

    cout << p.x() << "," << p.y() << endl;

    if (ui.derivedImages->currentIndex() > 0) {
        // g_imageParticlesAlgo->ProbeDerivedImage(ui.derivedImages->currentText().toUtf8().data());
    }
    
//    int xyRes = 10; //::round(::sqrt(m_Props.GetInt("numberOfPoints", 100)));
//    vtkPolyData* plane = m_PropScene.CreatePlane(xyRes, xyRes, p.x(), p.y());
//    plane->Print(cout);
//
//    m_PropScene.AddPolyData("plane", plane);
//    m_PropScene.SetColor(0, 0, 1);
//    m_PropScene.SetRepresentation(1);
//    m_Renderer->ResetCamera();
//    m_Interactor->Render();
//
//    cout << "Render a plane ..." << endl;

//    g_imageParticlesAlgo = ImageParticlesAlgorithm::New();
//    g_imageParticlesAlgo->SetPropertyAccess(m_Props);
//    g_imageParticlesAlgo->SetViewingDimension(GetCurrentView());
//    g_imageParticlesAlgo->SetImageList(&m_ImageList);
//    g_imageParticlesAlgo->SetEventCallback(this);
//    // g_imageParticlesAlgo->CreateRandomInitialPoints(m_Props.GetInt("numberOfPoints", 100));
//    g_imageParticlesAlgo->CreateInitialPoints(plane->GetPoints());
//    g_imageParticlesAlgo->SetSliceMarker(ui.sliceIndex->value());
    updateScene();
}
