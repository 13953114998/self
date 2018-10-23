include $(TOPDIR)/rules.mk

PKG_NAME:=base_master
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=git@gitee.com:innovation_bank/mqtt_base_master.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=HEAD
#4d8818757671177d2d4cf1208f545a6939be07ff
PKG_BUILD_DIR:=$(BUILD_DIR)/base_master-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/base_master
  SECTION:=base
  CATEGORY:=Utilities
  DEFAULT:=n
  TITLE:=master for base band
  URL:=http://innobank.com.cn
  DEPENDS:=+liblua +libncursesw +libreadline +libcurl +libpthread +libmosquitto +inno_crond +libsqlite3 +check_netstatus \
	  +lua +lua-cjson +lua-examples +luac +luarocks +luasec +luasocket +libuci
endef

define Package/base_master/description
	MQTT clients(sensor)and baseband master
endef

define Package/base_master/config
        source "$(SOURCE)/Config.in"
endef
#ifdef CONFIG_USELUA
#	CONFIG_USELUA="ON"
#else
#	CONFIG_USELUA="OFF"
#endif
ifdef CONFIG_CLI_SUPORT
	CONFIG_CLI_SUPORT="ON"
else
	CONFIG_CLI_SUPORT="OFF"
endif
ifdef CONFIG_SYSX86
	CONFIG_SYSX86="ON"
else
	CONFIG_SYSX86="OFF"
endif

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
		CFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
		CXXFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(EXTRA_LDFLAGS)" \
		cmake \
			-DCLI=$(CONFIG_CLI_SUPORT) \
			-DVMWORK="OFF" \
			-DIN_OPENWRT=1 \
			-DDEBUG="ON" \
			-DSYS86=$(CONFIG_SYSX86) \
			-DCMAKE_SYSTEM_NAME=Linux \
			-DCMAKE_SYSTEM_VERSION=1 \
			-DCMAKE_SYSTEM_PROCESSOR=$(ARCH) \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_C_FLAGS_RELEASE="-DNDEBUG" \
			-DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG" \
			-DCMAKE_C_COMPILER="$(CMAKE_C_COMPILER)" \
			-DCMAKE_C_COMPILER_ARG1="$(CMAKE_C_COMPILER_ARG1)" \
			-DCMAKE_CXX_COMPILER="$(CMAKE_CXX_COMPILER)" \
			-DCMAKE_CXX_COMPILER_ARG1="$(CMAKE_CXX_COMPILER_ARG1)" \
			-DCMAKE_EXE_LINKER_FLAGS:STRING="$(TARGET_LDFLAGS)" \
			-DCMAKE_MODULE_LINKER_FLAGS:STRING="$(TARGET_LDFLAGS)" \
			-DCMAKE_SHARED_LINKER_FLAGS:STRING="$(TARGET_LDFLAGS)" \
			-DCMAKE_AR="$(CMAKE_AR)" \
			-DCMAKE_RANLIB="$(CMAKE_RANLIB)" \
			-DCMAKE_FIND_ROOT_PATH="$(STAGING_DIR);$(TOOLCHAIN_DIR)" \
			-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
			-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
			-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
			-DCMAKE_STRIP=: \
			-DCMAKE_INSTALL_PREFIX=/usr \
			-DDL_LIBRARY=$(STAGING_DIR) \
			-DCMAKE_PREFIX_PATH=$(STAGING_DIR) \
			$(CMAKE_OPTIONS) \
		$(CMAKE_SOURCE_DIR) \
	)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)
endef

define Package/base_master/install
	$(INSTALL_DIR) $(1)/
	$(INSTALL_DIR) $(1)/usr/bin/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/base_master $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/recev_imsi_udp $(1)/usr/bin/
	if [ $(CONFIG_CLI_SUPORT) == "ON" ]; then \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/cli $(1)/usr/bin/; \
	fi
	$(CP) $(PKG_BUILD_DIR)/root/* $(1)/
endef

$(eval $(call BuildPackage,base_master))
