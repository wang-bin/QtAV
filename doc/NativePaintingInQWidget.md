Sometimes, we may want paint in QWidget without QPainter, for example, using DirectX api. This document will tell you how to reach it.

###0. setAttribute(Qt::WA_PaintOnScreen, true)

Document says that if you want to use native painting, the attribute should be true. Otherwise you will see the fuck flicker. 

###1. Let paint engine be 0

Thus QPainter is disabled. Otherwise you will see the fuck flicker again.  
This page is helpful: http://lists.trolltech.com/qt4-preview-feedback/2005-04/thread00609-0.html  
It seems that let paintEngine() return 0 is from here.

###2. Reimplement `paintEvent`

Oberviously, you should do something here. You can begin the painting in paintEvent

###3. Reimplement `showEvent`

This is important if you change the `WindowStaysOnTopHint` flag. As far as i know, some paint engines use the resource depend on the window. If window changes, resource must be recreated again. For example, render target in direct2d, device context in gdi+. If the window flag `WindowStaysOnTopHint` changes, the widget will hide then show again, maybe that's the reason. If you forget this step, then the widget will never update again after the `WindowStaysOnTopHint` changes.
