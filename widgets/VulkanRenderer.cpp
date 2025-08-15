#include "VulkanRenderer.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QResizeEvent>
#include "VulkanRendererPrivate.h"

namespace QtAV
{

	Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

	VulkanRenderer::VulkanRenderer()
		: VideoRenderer(*new VulkanRendererPrivate)
	{
		setMouseGrabEnabled(true);
		initVulkanInstance();
		setPreferredPixelFormat(VideoFormat::Format_YUV420P);
	}

	VulkanRenderer::~VulkanRenderer()
	{
	}

	VideoRendererId VulkanRenderer::id() const
	{
		return VideoRendererId_Vulkan;
	}

	bool VulkanRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
	{
		switch (pixfmt)
		{
		case QtAV::VideoFormat::Format_YUV444P:
		case QtAV::VideoFormat::Format_YUV422P:
		case QtAV::VideoFormat::Format_YUV420P:
		case QtAV::VideoFormat::Format_YUV411P:
		case QtAV::VideoFormat::Format_YUV410P:
			return true;
		default:
			break;
		}

		return false;
	}

	QWidget* VulkanRenderer::widget()
	{
		DPTR_D(VulkanRenderer);

		if (!d.widget)
		{
			d.widget = QWidget::createWindowContainer(this);
		}

		return d.widget;
	}

	QVulkanWindowRenderer* VulkanRenderer::createRenderer()
	{
		DPTR_D(VulkanRenderer);

		d.vulkanWindowRenderer = new VulkanWindowRenderer(this);
		return d.vulkanWindowRenderer;
	}

	bool VulkanRenderer::receiveFrame(const VideoFrame& frame)
	{
		DPTR_D(VulkanRenderer);

		d.vulkanWindowRenderer->setCurrentFrame(frame);
		updateUi();
		return true;
	}

	void VulkanRenderer::drawFrame()
	{
	}

	void VulkanRenderer::updateUi()
	{
		QCoreApplication::instance()->postEvent(this, new QEvent(QEvent::UpdateRequest));
		VideoRenderer::updateUi();
	}

	void VulkanRenderer::resizeEvent(QResizeEvent* ev)
	{
		DPTR_D(VulkanRenderer);

		resizeRenderer(ev->size().width(), ev->size().height());
		d.setupAspectRatio();
		QVulkanWindow::resizeEvent(ev);
	}

	bool VulkanRenderer::onSetBrightness(qreal brightness)
	{
		DPTR_D(VulkanRenderer);

		d.colorTransform.setBrightness(brightness);
		d.vulkanWindowRenderer->setColorTransformMatrix(d.colorTransform.matrixRef());
		return true;
	}

	bool VulkanRenderer::onSetContrast(qreal contrast)
	{
		DPTR_D(VulkanRenderer);

		d.colorTransform.setContrast(contrast);
		d.vulkanWindowRenderer->setColorTransformMatrix(d.colorTransform.matrixRef());
		return true;
	}

	bool VulkanRenderer::onSetHue(qreal hue)
	{
		DPTR_D(VulkanRenderer);

		d.colorTransform.setHue(hue);
		d.vulkanWindowRenderer->setColorTransformMatrix(d.colorTransform.matrixRef());
		return true;
	}

	bool VulkanRenderer::onSetSaturation(qreal saturation)
	{
		DPTR_D(VulkanRenderer);

		d.colorTransform.setSaturation(saturation);
		d.vulkanWindowRenderer->setColorTransformMatrix(d.colorTransform.matrixRef());
		return true;
	}

	void VulkanRenderer::onSetOutAspectRatioMode(OutAspectRatioMode mode)
	{
		DPTR_D(VulkanRenderer);

		d.setupAspectRatio();
	}

	void VulkanRenderer::onSetOutAspectRatio(qreal ratio)
	{
		DPTR_D(VulkanRenderer);

		d.setupAspectRatio();
	}

	bool VulkanRenderer::onSetOrientation(int value)
	{
		DPTR_D(VulkanRenderer);

		d.setupAspectRatio();
		return true;
	}

	void VulkanRenderer::initVulkanInstance()
	{
		DPTR_D(VulkanRenderer);

#ifdef  QT_DEBUG
		QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
		d.vulkanInstance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
#endif //  QT_DEBUG

		d.vulkanInstance.create();
		setVulkanInstance(&d.vulkanInstance);
	}
}