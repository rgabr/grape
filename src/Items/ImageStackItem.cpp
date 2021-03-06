/* ImageStackItem.cpp
 *
 Copyright (C) 2016 UTHealth MRI Research
*
* This file is part of the GRAPE Diagram Editor.
*
* GRAPE is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* GRAPE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GRAPE.  If not, see <http://www.gnu.org/licenses/>
 */

#include <ImageStackItem.h>
#include <Diagram.h>
#include <DiagramItemPoint.h>
#include <DiagramWidgets.h>

//--------------------------------------------------------------------------------------------------
ImageStackItem::ImageStackItem(const NodeInterface &NI) : NodeItem(NI)
{
}
//--------------------------------------------------------------------------------------------------
ImageStackItem::ImageStackItem(const ImageStackItem& item) : NodeItem(item) { }
//--------------------------------------------------------------------------------------------------
ImageStackItem::~ImageStackItem() { }
//--------------------------------------------------------------------------------------------------
DiagramItem* ImageStackItem::copy() const
{
    return new ImageStackItem(this->getNi());
}
//--------------------------------------------------------------------------------------------------
QString ImageStackItem::uniqueKey() const
{
    return "ImageStack";
}
//--------------------------------------------------------------------------------------------------
QString ImageStackItem::iconPath() const
{
    return "";//":/icons/oxygen/IR.png";
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::setCaption(const QString& caption)
{
    setPropertyValue("Caption", QVariant(caption));
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::setFont(const QFont& font)
{
	setPropertyValue("Font", QVariant(font));
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::setTextBrush(const QBrush& brush)
{
	setPropertyValue("Text Brush", QVariant(brush));
}
//--------------------------------------------------------------------------------------------------
QString ImageStackItem::caption() const
{
    return propertyValue("Caption").toString();
}
//--------------------------------------------------------------------------------------------------
QString ImageStackItem::caption1()
{
    return propertyValue("Caption").toString();
}
//--------------------------------------------------------------------------------------------------
QFont ImageStackItem::font() const
{
	return propertyValue("Font").value<QFont>();
}
//--------------------------------------------------------------------------------------------------
QBrush ImageStackItem::textBrush() const
{
	return propertyValue("Text Brush").value<QBrush>();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::markDirty()
{
	mBoundingRect = QRectF();
}
//--------------------------------------------------------------------------------------------------
QRectF ImageStackItem::boundingRect() const
{
    return DiagramRoundItem::boundingRect().united(mBoundingRect);
}
//--------------------------------------------------------------------------------------------------
QPainterPath ImageStackItem::shape() const
{
    QPainterPath shape = DiagramRoundItem::shape();

	if (brush().color().alpha() == 0)
		shape.addRect(mBoundingRect);
	return shape;
}
//--------------------------------------------------------------------------------------------------
bool ImageStackItem::isSuperfluous() const
{
    return (DiagramRoundItem::isSuperfluous() && caption() == "");
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::render(DiagramPainter* painter)
{
    QFont lFont = font();
	qreal scaleFactor = 1.0 / unitsScale(units(), UnitsMils);
	qreal deviceFactor = 1.0;

    NodeItem::render(painter);

	if (painter->paintEngine()->paintDevice())
        deviceFactor = 96.0 / painter->paintEngine()->paintDevice()->logicalDpiX();

	lFont.setPointSizeF(lFont.pointSizeF() * 0.72 / scaleFactor);       // Scale to workspace
	lFont.setPointSizeF(lFont.pointSizeF() * deviceFactor);             // Scale to device

	if (!mBoundingRect.isValid()) updateLabel(lFont, painter->device());

	painter->setFont(lFont);
	painter->setForItem(QPen(textBrush(), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin), textBrush());
    painter->translate(DiagramRoundItem::boundingRect().center());
	painter->rotate(orientedTextAngle());
	painter->scale(scaleFactor, scaleFactor);
    painter->drawText(mTextRect, Qt::AlignCenter, caption());
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::writeXmlAttributes(QXmlStreamWriter& xmlWriter, const QList<DiagramItem*>& items)
{
    DiagramRoundItem::writeXmlAttributes(xmlWriter, items);

    xmlWriter.writeAttribute("caption", caption());
    xmlWriter.writeAttribute("font", fontToString(font()));
	xmlWriter.writeAttribute("textBrush", brushToString(textBrush()));
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::readXmlAttributes(QXmlStreamReader& xmlReader, const QList<DiagramItem*>& items)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();

    DiagramRoundItem::readXmlAttributes(xmlReader, items);
    if (attributes.hasAttribute("caption"))
        setCaption(attributes.value("caption").toString());
    if (attributes.hasAttribute("font"))
		setFont(fontFromString(attributes.value("font").toString()));
	if (attributes.hasAttribute("textBrush"))
		setTextBrush(brushFromString(attributes.value("textBrush").toString()));
}
//--------------------------------------------------------------------------------------------------
DiagramItemPropertiesDialog* ImageStackItem::propertiesDialog(QWidget* parent) const
{
	if (parent == NULL) parent = diagram();
    return new ImageStackItemDialog(parent);
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::rotateEvent(const QPointF& diagramPos)
{
    DiagramRoundItem::rotateEvent(diagramPos);
	markDirty();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::rotateBackEvent(const QPointF& diagramPos)
{
    DiagramRoundItem::rotateBackEvent(diagramPos);
	markDirty();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::flipEvent(const QPointF& diagramPos)
{
    DiagramRoundItem::flipEvent(diagramPos);
	markDirty();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::resizeEvent(DiagramItemPoint* itemPoint, const QPointF& parentPos)
{
    NodeItem::resizeEvent(itemPoint, parentPos);
	markDirty();
}
//--------------------------------------------------------------------------------------------------
QVariant ImageStackItem::aboutToChangeEvent(Reason reason, const QVariant& value)
{
    QVariant result = DiagramRoundItem::aboutToChangeEvent(reason, value);
	if (reason == UnitsChange)
	{
		DiagramUnits newUnits = (DiagramUnits)value.toInt();
		qreal scaleFactor = unitsScale(units(), newUnits);
		QFont lFont = font();

		lFont.setPointSizeF(lFont.pointSizeF() * scaleFactor);
		setFont(lFont);
	}
	return result;
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::changedEvent(Reason reason, const QVariant& value)
{
	Q_UNUSED(value);

	if (reason == UnitsChange || reason == PropertyChange) markDirty();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItem::updateLabel(const QFont& font, QPaintDevice* device)
{
	QSizeF textSizeF = textSize(caption(), font, device);
	qreal scaleFactor = 1.0 / unitsScale(units(), UnitsMils);

	// Update mTextRect
	mTextRect = QRectF(QPointF(-textSizeF.width() / 2, -textSizeF.height() / 2), textSizeF);

	// Update mBoundingRect
	mBoundingRect = rotateRect(QRectF(
		QPointF(mTextRect.left() * scaleFactor, mTextRect.top() * scaleFactor),
		QPointF(mTextRect.right() * scaleFactor, mTextRect.bottom() * scaleFactor)),
        rotationAngle()).translated(DiagramRoundItem::boundingRect().center());
}
//--------------------------------------------------------------------------------------------------
qreal ImageStackItem::orientedTextAngle() const
{
	qreal textAngle = rotationAngle();

	if (isFlipped())
	{
		if (textAngle == 90) textAngle = 270;
		else if (textAngle == 270) textAngle = 90;
	}
	return textAngle;
}
//--------------------------------------------------------------------------------------------------
ImageStackItemDialog::ImageStackItemDialog(QWidget* parent) : DiagramItemPropertiesDialog(parent)
{
	QSizeF diagramSize = diagramSizeFromParent(parent);

	fontToolBar = new DiagramFontToolBar(
		DiagramFontToolBar::ShowFont | DiagramFontToolBar::ShowBrush, diagramSize);
    captionEdit = new QTextEdit();
	topLeftLayout = new DiagramPositionLayout(diagramSize);
	bottomRightLayout = new DiagramPositionLayout(diagramSize);
	penGroup = new DiagramPenGroup(DiagramPenGroup::ShowAll, diagramSize);
	brushGroup = new DiagramBrushGroup(DiagramBrushGroup::ShowColor);
	QGroupBox* positionGroup = new QGroupBox("Position");
	QFormLayout* fLayout = new QFormLayout();
	fLayout->addRow("Top Left: ", topLeftLayout);
	fLayout->addRow("Bottom Right: ", bottomRightLayout);
	fLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
	fLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	fLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	fLayout->itemAt(0, QFormLayout::LabelRole)->widget()->setMinimumWidth(80);
	positionGroup->setLayout(fLayout);
	QGroupBox* textGroup = new QGroupBox("Text");
	QVBoxLayout* vLayout = new QVBoxLayout();
	vLayout->addWidget(fontToolBar);
	vLayout->addWidget(captionEdit, 100);
	textGroup->setLayout(vLayout);
    QGroupBox* ImageStackGroup = new QGroupBox("Parameter");
    QFormLayout* pLayout = new QFormLayout();
    pLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    pLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    ImageStackGroup->setLayout(pLayout);

    addWidget(positionGroup);
    addWidget(textGroup);
    addWidget(ImageStackGroup);
    addWidget(penGroup);
    addWidget(brushGroup);
    addWidget(new QWidget(), 100);
    addButtonBox();
	captionEdit->setFocus();

    setWindowTitle("ImageStack  Properties");
	resize(280, 10);
}
//--------------------------------------------------------------------------------------------------
ImageStackItemDialog::~ImageStackItemDialog() { }
//--------------------------------------------------------------------------------------------------
void ImageStackItemDialog::setProperties(const QHash<QString, QVariant>& properties)
{
	if (properties.contains("Pen")) penGroup->setPen(properties["Pen"].value<QPen>());
	if (properties.contains("Brush")) brushGroup->setBrush(properties["Brush"].value<QBrush>());
	if (properties.contains("Caption")) captionEdit->setText(properties["Caption"].toString());
	if (properties.contains("Font")) fontToolBar->setFont(properties["Font"].value<QFont>());
	if (properties.contains("Text Brush"))
		fontToolBar->setBrush(properties["Text Brush"].value<QBrush>());
    captionEdit->selectAll();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItemDialog::setPointPositions(const QList<QPointF>& pointPositions)
{
	if (pointPositions.size() >= 2)
	{
		topLeftLayout->setPosition(pointPositions[0]);
		bottomRightLayout->setPosition(pointPositions[1]);
	}
}
//--------------------------------------------------------------------------------------------------
QHash<QString, QVariant> ImageStackItemDialog::properties() const
{
	QHash<QString, QVariant> properties;

	properties["Pen"] = QVariant(penGroup->pen());
	properties["Brush"] = QVariant(brushGroup->brush());
	properties["Caption"] = QVariant(captionEdit->toPlainText());
	properties["Font"] = QVariant(fontToolBar->font());
	properties["Text Brush"] = QVariant(fontToolBar->brush());
	return properties;
}
//--------------------------------------------------------------------------------------------------
QList<QPointF> ImageStackItemDialog::pointPositions() const
{
	QList<QPointF> pointPositions;

	pointPositions.append(topLeftLayout->position());
	pointPositions.append(bottomRightLayout->position());

	return pointPositions;
}
//--------------------------------------------------------------------------------------------------
void ImageStackItemDialog::accept()
{
	if (!captionEdit->document()->toPlainText().isEmpty()) DiagramItemPropertiesDialog::accept();
}
//--------------------------------------------------------------------------------------------------
void ImageStackItemDialog::hideEvent(QHideEvent* event)
{
	QDialog::hideEvent(event);
	captionEdit->setFocus();
}
//--------------------------------------------------------------------------------------------------
