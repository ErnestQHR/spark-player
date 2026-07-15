QT += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += \
    main.cpp \
    player.cpp \

HEADERS += \
    clickableslider.h \
    player.h \

FORMS += \
    player.ui 

RESOURCES += \
    resources.qrc 