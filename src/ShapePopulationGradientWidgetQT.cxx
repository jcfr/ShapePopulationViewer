
// ShapePopulationViewer includes
#include "ShapePopulationColorMapIO.h"
#include "ShapePopulationGradientWidgetQT.h"

// CTK includes
#ifdef ShapePopulationViewer_BUILD_SLICER_EXTENSION
#include <ctkTransferFunction.h>
#include <ctkTransferFunctionGradientItem.h>
#include <ctkTransferFunctionControlPointsItem.h>
#include <ctkVTKColorTransferFunction.h>
#include <ctkVTKScalarsToColorsView.h>

// VTK includes
#include <vtkAxis.h>
#include <vtkColorTransferFunction.h>
#include <vtkContextScene.h>
#include <vtkControlPointsItem.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>

// Qt includes
#include <QDoubleSpinBox>
#include <QHBoxLayout>

#endif

class ShapePopulationGradientWidgetQTPrivate
{
public:
    ShapePopulationGradientWidgetQTPrivate();

#ifdef ShapePopulationViewer_BUILD_SLICER_EXTENSION
    vtkSmartPointer<vtkColorTransferFunction> CTF;
    QDoubleSpinBox* XSpinBox;
#endif
};

ShapePopulationGradientWidgetQTPrivate::ShapePopulationGradientWidgetQTPrivate()
{
}

ShapePopulationGradientWidgetQT::ShapePopulationGradientWidgetQT(QWidget* parent)
    : Superclass(parent), d_ptr( new ShapePopulationGradientWidgetQTPrivate )
{
#ifdef ShapePopulationViewer_BUILD_SLICER_EXTENSION
    Q_D(ShapePopulationGradientWidgetQT);
    this->setHorizontalSliderVisible(false);
    this->setVerticalSliderVisible(false);

    // Hide controls
    QHBoxLayout* topLayout = this->findChild<QHBoxLayout*>("TopLayout");
    for(int idx = 0; idx < topLayout->count(); ++idx)
    {
        QWidgetItem* widgetItem = dynamic_cast<QWidgetItem*>(topLayout->itemAt(idx));
        if (widgetItem)
        {
            widgetItem->widget()->setVisible(false);
        }
    }

    // Configure spinbox
    d->XSpinBox = this->findChild<QDoubleSpinBox*>("XSpinBox");
    d->XSpinBox->setDecimals(6);
    d->XSpinBox->setKeyboardTracking(false);

    d->CTF = vtkSmartPointer<vtkColorTransferFunction>::New();
    this->view()->addColorTransferFunction(d->CTF);

    // Lookup and set vtkControlPointsItem
    // TODO: Update ctkVTKScalarsToColorsWidget::onPlotAdded to automatically set it
    foreach(vtkPlot* plot, this->view()->plots())
    {
        vtkControlPointsItem * controlPointsItem = vtkControlPointsItem::SafeDownCast(plot);
        if (controlPointsItem)
        {
            this->setCurrentControlPointsItem(controlPointsItem);
            break;
        }
    }

    this->qvtkConnect(this->currentControlPointsItem(), vtkControlPointsItem::CurrentPointChangedEvent,
                      this, SLOT(onCurrentPointChangedEvent(vtkObject*)));
    this->qvtkConnect(this->currentControlPointsItem(), vtkCommand::ModifiedEvent,
                      this, SLOT(onCurrentPointChangedEvent(vtkObject*)));
#endif
}

ShapePopulationGradientWidgetQT::~ShapePopulationGradientWidgetQT()
{
}

#ifdef ShapePopulationViewer_BUILD_SLICER_EXTENSION

void ShapePopulationGradientWidgetQT::loadColorPointList(QString a_filePath, std::vector<colorPointStruct> * a_colorPointList)
{
    ShapePopulationColorMapIO::loadColorPointList(a_filePath, a_colorPointList, this);
    this->setAllColors(a_colorPointList);
}

void ShapePopulationGradientWidgetQT::saveColorPointList(QString a_filePath)
{
    //Get ColorPointList
    std::vector<colorPointStruct> colorPointList;
    getAllColors(&colorPointList);
    ShapePopulationColorMapIO::saveColorPointList(a_filePath, colorPointList, this);
}

bool ShapePopulationGradientWidgetQT::getAllColors(std::vector<colorPointStruct> * colorPointList)
{
    Q_D(ShapePopulationGradientWidgetQT);
    colorPointList->clear();

    for(int idx = 0; idx < d->CTF->GetSize(); ++idx)
    {
        double nodeValue[6];
        d->CTF->GetNodeValue(idx, nodeValue);
        colorPointStruct colorPoint(nodeValue[0], nodeValue[1], nodeValue[2], nodeValue[3]);
        colorPointList->push_back(colorPoint);
    }
    return d->CTF->GetSize() > 0;
}

void ShapePopulationGradientWidgetQT::setAllColors(std::vector<colorPointStruct> * colorPointList)
{
    Q_D(ShapePopulationGradientWidgetQT);
    d->CTF->RemoveAllPoints();
    for(unsigned int idx = 0 ; idx < colorPointList->size() ; idx++)
    {
        colorPointStruct colorPoint = colorPointList->at(idx);
        d->CTF->AddRGBPoint(colorPoint.pos, colorPoint.r, colorPoint.g, colorPoint.b);
    }
}

void ShapePopulationGradientWidgetQT::setFocusIndex(int index)
{
    if (index < -1 || index >= this->currentControlPointsItem()->GetNumberOfPoints())
    {
        return;
    }
    this->currentControlPointsItem()->SetCurrentPoint(index);
}

int ShapePopulationGradientWidgetQT::getFocusIndex()
{
    return static_cast<int>(this->currentControlPointsItem()->GetCurrentPoint());
}

void ShapePopulationGradientWidgetQT::enable(std::vector<colorPointStruct> *a_colorPointList)
{
    this->setEnabled(true);
    this->setAllColors(a_colorPointList);
}

void ShapePopulationGradientWidgetQT::disable()
{
    this->setEnabled(false);
}

void ShapePopulationGradientWidgetQT::updateRange(double min, double max)
{
    Q_D(ShapePopulationGradientWidgetQT);

    double range = fabs(max - min);
    d->XSpinBox->setMinimum(min);
    d->XSpinBox->setMaximum(max);
    d->XSpinBox->setSingleStep(range / 100);

    {
        // min
        double point[4];
        this->currentControlPointsItem()->GetControlPoint(0, point);
        point[0] = min;
        this->currentControlPointsItem()->SetControlPoint(0, point);

        double ctfPoint[6] = {0.0};
        d->CTF->GetNodeValue(0, ctfPoint);
        ctfPoint[0] = min;
        d->CTF->SetNodeValue(0, ctfPoint);
    }
    {
        // max
        int maxIndex = d->CTF->GetSize() - 1;
        double point[4];
        this->currentControlPointsItem()->GetControlPoint(maxIndex, point);
        point[maxIndex] = max;
        this->currentControlPointsItem()->SetControlPoint(maxIndex, point);

        double ctfPoint[6] = {0.0};
        d->CTF->GetNodeValue(maxIndex, ctfPoint);
        ctfPoint[0] = max;
        d->CTF->SetNodeValue(maxIndex, ctfPoint);
    }
    // Elements: {leftMin, leftMax, bottomMin, bottomMax, rightMin, rightMax, topMin, topMax}
    // (probably according to vtkAxis::Location)
    double chartBounds[8] = {0.0};
    this->view()->chartBounds(chartBounds);
    chartBounds[2] = min;
    chartBounds[3] = max;
    this->view()->setChartUserBounds(chartBounds);
    this->view()->update();
    this->view()->setAxesToChartBounds();
}

void ShapePopulationGradientWidgetQT::onCurrentPointChangedEvent(vtkObject* caller)
{
    vtkControlPointsItem* controlPoints = vtkControlPointsItem::SafeDownCast(caller);
    long newPoint = this->getFocusIndex();
    if (!controlPoints || newPoint <= -1)
      {
      return;
      }
    emit colorsChanged();
}

#endif
