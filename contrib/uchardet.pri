UCHARDET_SRC = $$PWD/uchardet/src
INCLUDEPATH += $$UCHARDET_SRC
HEADERS = \
    $$UCHARDET_SRC/uchardet.h \
    $$UCHARDET_SRC/nsMBCSGroupProber.h \
    $$UCHARDET_SRC/nsGB2312Prober.h \
    $$UCHARDET_SRC/nsUTF8Prober.h \
    $$UCHARDET_SRC/nsBig5Prober.h \
    $$UCHARDET_SRC/nsEUCTWProber.h \
    $$UCHARDET_SRC/nsCharSetProber.h \
    $$UCHARDET_SRC/nsSBCSGroupProber.h \
    $$UCHARDET_SRC/nsCodingStateMachine.h \
    $$UCHARDET_SRC/nsEUCKRProber.h \
    $$UCHARDET_SRC/nsUniversalDetector.h \
    $$UCHARDET_SRC/nsSJISProber.h \
    $$UCHARDET_SRC/nsLatin1Prober.h \
    $$UCHARDET_SRC/nsSBCharSetProber.h \
    $$UCHARDET_SRC/nscore.h \
    $$UCHARDET_SRC/CharDistribution.h \
    $$UCHARDET_SRC/nsPkgInt.h \
    $$UCHARDET_SRC/nsEscCharsetProber.h \
    $$UCHARDET_SRC/nsHebrewProber.h \
    $$UCHARDET_SRC/prmem.h \
    $$UCHARDET_SRC/JpCntx.h \
    $$UCHARDET_SRC/nsEUCJPProber.h

SOURCES = \
    $$UCHARDET_SRC/uchardet.cpp \
    $$UCHARDET_SRC/nsEUCTWProber.cpp \
    $$UCHARDET_SRC/LangCyrillicModel.cpp \
    $$UCHARDET_SRC/nsSBCharSetProber.cpp \
    $$UCHARDET_SRC/CharDistribution.cpp \
    $$UCHARDET_SRC/nsLatin1Prober.cpp \
    $$UCHARDET_SRC/nsEscSM.cpp \
    $$UCHARDET_SRC/nsUTF8Prober.cpp \
    $$UCHARDET_SRC/nsBig5Prober.cpp \
    $$UCHARDET_SRC/LangBulgarianModel.cpp \
    $$UCHARDET_SRC/nsGB2312Prober.cpp \
    $$UCHARDET_SRC/nsCharSetProber.cpp \
    $$UCHARDET_SRC/nsEscCharsetProber.cpp \
    $$UCHARDET_SRC/nsEUCKRProber.cpp \
    $$UCHARDET_SRC/nsEUCJPProber.cpp \
    $$UCHARDET_SRC/nsSJISProber.cpp \
    $$UCHARDET_SRC/nsMBCSSM.cpp \
    $$UCHARDET_SRC/nsHebrewProber.cpp \
    $$UCHARDET_SRC/nsUniversalDetector.cpp \
    $$UCHARDET_SRC/nsSBCSGroupProber.cpp \
    $$UCHARDET_SRC/nsMBCSGroupProber.cpp \
    $$UCHARDET_SRC/LangThaiModel.cpp \
    $$UCHARDET_SRC/LangHebrewModel.cpp \
    $$UCHARDET_SRC/JpCntx.cpp \
    $$UCHARDET_SRC/LangHungarianModel.cpp \
    $$UCHARDET_SRC/LangGreekModel.cpp
