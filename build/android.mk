# This Makefile fragment builds the Android package (XCSoar.apk).
# We're not using NDK's Makefiles because our Makefile is so big and
# complex, we don't want to duplicate that for another platform.

ifeq ($(TARGET),ANDROID)

ANT = ant
JAVAH = javah
JARSIGNER = jarsigner
ANDROID_KEYSTORE = $(HOME)/.android/mk.keystore
ANDROID_KEY_ALIAS = mk
ANDROID_BUILD = $(TARGET_OUTPUT_DIR)/build
ANDROID_BIN = $(TARGET_OUTPUT_DIR)/bin

ANDROID_SDK ?= $(HOME)/opt/android-sdk-linux_x86
ANDROID_ABI = armeabi
ANDROID_LIB_DIR = /opt/android/libs/$(ANDROID_ABI)

ANDROID_LIB_NAMES = sdl-1.2
ifeq ($(OPENGL),n)
ANDROID_LIB_NAMES += sdl_gfx
endif

ANDROID_LIB_FILES = $(patsubst %,$(ANDROID_LIB_DIR)/lib%.so,$(ANDROID_LIB_NAMES))
ANDROID_SO_FILES = $(patsubst %,$(ANDROID_BUILD)/libs/$(ANDROID_ABI)/lib%.so,$(ANDROID_LIB_NAMES))

ifneq ($(V),2)
ANT += -quiet
else
JARSIGNER += -verbose
endif

JAVA_PACKAGE = org.xcsoar
CLASS_NAME = $(JAVA_PACKAGE).NativeView
CLASS_SOURCE = $(subst .,/,$(CLASS_NAME)).java
CLASS_CLASS = $(patsubst %.java,%.class,$(CLASS_SOURCE))

NATIVE_CLASSES = NativeView EventBridge Timer
NATIVE_PREFIX = $(TARGET_OUTPUT_DIR)/include/$(subst .,_,$(JAVA_PACKAGE))_
NATIVE_HEADERS = $(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES))

JAVA_SOURCES = $(wildcard android/src/*.java)
JAVA_CLASSES = $(patsubst android/src/%.java,bin/classes/org/xcsoar/%.class,$(JAVA_SOURCES))

DRAWABLE_DIR = $(ANDROID_BUILD)/res/drawable

$(ANDROID_BUILD)/res/drawable/icon.png: $(OUT)/data/graphics/xcsoarswiftsplash_160.png | $(ANDROID_BUILD)/res/drawable/dirstamp
	$(Q)$(IM_PREFIX)convert -scale 48x48 $< $@

PNG1 := $(patsubst Data/bitmaps/%.bmp,$(DRAWABLE_DIR)/%.png,$(wildcard Data/bitmaps/*.bmp))
$(PNG1): $(DRAWABLE_DIR)/%.png: Data/bitmaps/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG2 := $(patsubst output/data/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_LAUNCH_FLY_224) $(BMP_LAUNCH_SIM_224))
$(PNG2): $(DRAWABLE_DIR)/%.png: output/data/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG3 := $(patsubst output/data/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_SPLASH_80) $(BMP_SPLASH_160) $(BMP_TITLE_110) $(BMP_TITLE_320))
$(PNG3): $(DRAWABLE_DIR)/%.png: output/data/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG4 := $(patsubst output/data/icons/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_ICONS) $(BMP_ICONS_160))
$(PNG4): $(DRAWABLE_DIR)/%.png: output/data/icons/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG_FILES = $(DRAWABLE_DIR)/icon.png $(PNG1) $(PNG2) $(PNG3) $(PNG4)

# symlink some important files to $(ANDROID_BUILD) and let the Android
# SDK generate build.xml
$(ANDROID_BUILD)/build.xml: android/AndroidManifest.xml $(PNG_FILES) | $(TARGET_OUTPUT_DIR)/bin/dirstamp
	@$(NQ)echo "  ANDROID $@"
	$(Q)rm -f $(@D)/AndroidManifest.xml $(@D)/src $(@D)/bin $(@D)/res/values
	$(Q)mkdir -p $(ANDROID_BUILD)/res
	$(Q)ln -s ../../../android/AndroidManifest.xml ../../../android/src ../bin $(@D)/
	$(Q)ln -s ../../../../android/res/values $(@D)/res/
	$(Q)$(ANDROID_SDK)/tools/android update project --path $(@D) --target $(ANDROID_PLATFORM)
	@touch $@

# add dependency to this source file
$(TARGET_OUTPUT_DIR)/$(SRC)/Android/Main.o: $(NATIVE_HEADERS)
$(TARGET_OUTPUT_DIR)/$(SRC)/Android/EventBridge.o: $(NATIVE_HEADERS)
$(TARGET_OUTPUT_DIR)/$(SRC)/Android/Timer.o: $(NATIVE_HEADERS)

$(ANDROID_BUILD)/libs/$(ANDROID_ABI)/libapplication.so: $(TARGET_OUTPUT_DIR)/bin/xcsoar.so | $(ANDROID_BUILD)/libs/$(ANDROID_ABI)/dirstamp
	cp $< $@

$(ANDROID_SO_FILES): $(ANDROID_BUILD)/libs/$(ANDROID_ABI)/lib%.so: $(ANDROID_LIB_DIR)/lib%.so
	cp $< $@

$(ANDROID_BIN)/XCSoar-debug.apk: $(ANDROID_BUILD)/libs/$(ANDROID_ABI)/libapplication.so $(ANDROID_SO_FILES) $(ANDROID_BUILD)/build.xml $(ANDROID_BUILD)/res/drawable/icon.png android/src/*.java
	@$(NQ)echo "  ANT     $@"
	$(Q)cd $(ANDROID_BUILD) && $(ANT) debug

$(ANDROID_BIN)/classes/$(CLASS_CLASS): $(JAVA_SOURCES) $(ANDROID_BUILD)/build.xml
	@$(NQ)echo "  ANT     $@"
	$(Q)cd $(ANDROID_BUILD) && $(ANT) compile

$(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES)): $(NATIVE_PREFIX)%.h: $(ANDROID_BIN)/classes/$(CLASS_CLASS)
	@$(NQ)echo "  JAVAH   $@"
	$(Q)javah -classpath $(ANDROID_BIN)/classes -d $(@D) $(subst _,.,$(patsubst $(patsubst ./%,%,$(TARGET_OUTPUT_DIR))/include/%.h,%,$@))

$(ANDROID_BIN)/XCSoar-unsigned.apk: $(ANDROID_BUILD)/libs/$(ANDROID_ABI)/libapplication.so $(ANDROID_SO_FILES) $(ANDROID_BUILD)/build.xml $(ANDROID_BUILD)/res/drawable/icon.png android/src/*.java
	@$(NQ)echo "  ANT     $@"
	$(Q)cd $(ANDROID_BUILD) && $(ANT) release

$(ANDROID_BIN)/XCSoar.apk: $(ANDROID_BIN)/XCSoar-unsigned.apk
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(JARSIGNER) -keystore $(ANDROID_KEYSTORE) -signedjar $(ANDROID_BIN)/XCSoar.apk $(ANDROID_BIN)/XCSoar-unsigned.apk $(ANDROID_KEY_ALIAS)

endif
