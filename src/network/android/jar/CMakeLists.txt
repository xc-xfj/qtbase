# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt5/android/network/QtNetwork.java
)

qt_internal_add_jar(QtAndroidNetwork
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

install_jar(QtAndroidNetwork
    DESTINATION jar
    COMPONENT Devel
)

