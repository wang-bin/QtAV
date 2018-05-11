import sys

from PyQt5 import QtCore, QtGui, QtWidgets
import QtAV, QtAVWidgets


class PlayerWindow(QtWidgets.QWidget):
   def __init__(self, parent = None):
      QtWidgets.QWidget.__init__(self, parent)

      self.m_unit = 1000.0
      self.setWindowTitle("QtAV simple player example")
      self.m_player = QtAV.AVPlayer(self)
      vl = QtWidgets.QVBoxLayout()
      self.setLayout(vl)
      self.m_vo = QtAV.VideoOutput(self)

      if self.m_vo.widget() is None:
         QtWidgets.QMessageBox.warning(0, "QtAV error", "Can not create video renderer")
         return

      self.m_player.setRenderer(self.m_vo)
      vl.addWidget(self.m_vo.widget())
      self.m_slider = QtWidgets.QSlider()
      self.m_slider.setOrientation(QtCore.Qt.Horizontal)
      self.m_slider.sliderMoved[int].connect(self.seekBySliderVal)
      self.m_slider.sliderPressed.connect(self.seekBySlider)
      self.m_player.positionChanged.connect(self.updateSliderVal)
      self.m_player.started.connect(self.updateSlider)
      self.m_player.notifyIntervalChanged.connect(self.updateSliderUnit)

      vl.addWidget(self.m_slider)
      hb = QtWidgets.QHBoxLayout()
      vl.addLayout(hb)
      self.m_openBtn = QtWidgets.QPushButton("Open")
      self.m_playBtn = QtWidgets.QPushButton("Play/Pause")
      self.m_stopBtn = QtWidgets.QPushButton("Stop")
      hb.addWidget(self.m_openBtn)
      hb.addWidget(self.m_playBtn)
      hb.addWidget(self.m_stopBtn)
      self.m_openBtn.clicked.connect(self.openMedia)
      self.m_playBtn.clicked.connect(self.playPause)
      self.m_stopBtn.clicked.connect(self.m_player.stop)

   def openMedia(self):
      fname = QtWidgets.QFileDialog.getOpenFileName(None, "Open a video")[0]
      if not fname:
         return
      self.m_player.play(fname)

   def seekBySliderVal(self, value):
      if not self.m_player.isPlaying():
         return
      self.m_player.seek(int(value * self.m_unit))

   def seekBySlider(self):
      self.seekBySliderVal(self.m_slider.value())

   def playPause(self):
      if not self.m_player.isPlaying():
         self.m_player.play()
         return
      self.m_player.pause(not self.m_player.isPaused())

   def updateSliderVal(self, value):
      self.m_slider.setRange(0, int(self.m_player.duration() / self.m_unit))
      self.m_slider.setValue(int(value / self.m_unit))

   def updateSlider(self):
      self.updateSliderVal(self.m_player.position())

   def updateSliderUnit(self):
      self.m_unit = float(self.m_player.notifyInterval())
      self.updateSlider()


if __name__ == "__main__":
   QtAVWidgets.registerRenderers()
   app = QtWidgets.QApplication(sys.argv)
   player = PlayerWindow()
   player.show()
   player.resize(800, 600)
   app.exec_()
