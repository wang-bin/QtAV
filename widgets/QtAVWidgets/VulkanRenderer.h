#pragma once

#include <QWidget>
#include <QVulkanWindow>
#include <QtAVWidgets/global.h>
#include <QtAV/VideoRenderer.h>

namespace QtAV {
	class Q_AVWIDGETS_EXPORT VulkanRenderer : public QVulkanWindow, public VideoRenderer
	{
		DPTR_DECLARE_PRIVATE(VideoRenderer)

	public:
		VulkanRenderer();
		~VulkanRenderer();

		// VideoRenderer virtual function
		virtual VideoRendererId id() const override;
		virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const override;
		virtual QWidget* widget() override;

		// QVulkanWindow virtual function
		virtual QVulkanWindowRenderer* createRenderer() override;

	protected:
		virtual bool receiveFrame(const VideoFrame& frame) override;
		virtual void drawFrame() override;
		virtual void updateUi() override;

		virtual void resizeEvent(QResizeEvent* ev) override;

	private:
		virtual bool onSetBrightness(qreal brightness) override;
		virtual bool onSetContrast(qreal contrast) override;
		virtual bool onSetHue(qreal hue) override;
		virtual bool onSetSaturation(qreal saturation) override;

		virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode) override;
		virtual void onSetOutAspectRatio(qreal ratio) override;

		virtual bool onSetOrientation(int value) override;

	private:
		void initVulkanInstance();
	};
}