include $(TOPDIR)/rules.mk

PKG_NAME:=snode.c
PKG_VERSION:=$(or $(SNODEC_PACKAGE_VERSION),$(patsubst v%,%,$(filter v%,$(SNODEC_SOURCE_TAG))),2.0.0)
PKG_RELEASE:=$(or $(SNODEC_PACKAGE_RELEASE),1)
PKG_MIRROR_HASH:=$(or $(SNODEC_SOURCE_HASH),skip)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/SNodeC/snode.c.git
PKG_SOURCE_VERSION:=$(or $(SNODEC_SOURCE_TAG),v$(PKG_VERSION))

PKG_MAINTAINER:=Volker Christian <me@vchrist.at>
PKG_LICENSE:=LGPL-3.0-or-later
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=nlohmannjson openssl file bluez libmariadb
PKG_BUILD_PARALLEL:=1
PKG_CONFIG_DEPENDS:=CONFIG_SNODEC_GROUP_NAME \
  CONFIG_SNODEC_EPOLL \
  CONFIG_SNODEC_POLL \
  CONFIG_SNODEC_SELECT \
  CONFIG_SNODEC_READ_BLOCKSIZE \
  CONFIG_SNODEC_WRITE_BLOCKSIZE \
  CONFIG_SNODEC_READ_TIMEOUT \
  CONFIG_SNODEC_WRITE_TIMEOUT \
  CONFIG_SNODEC_MAXIMUM_WRITE_QUEUE_BYTES \
  CONFIG_SNODEC_WRITE_QUEUE_HIGH_WATERMARK \
  CONFIG_SNODEC_WRITE_QUEUE_LOW_WATERMARK \
  CONFIG_SNODEC_BACKLOG \
  CONFIG_SNODEC_ACCEPTS_PER_TICK \
  CONFIG_SNODEC_ACCEPT_TIMEOUT \
  CONFIG_SNODEC_CONNECT_TIMEOUT \
  CONFIG_SNODEC_TERMINATE_TIMEOUT \
  CONFIG_SNODEC_RECONNECT \
  CONFIG_SNODEC_RECONNECT_TIME \
  CONFIG_SNODEC_RETRY \
  CONFIG_SNODEC_RETRY_ON_FATAL \
  CONFIG_SNODEC_RETRY_TIMEOUT \
  CONFIG_SNODEC_RETRY_TRIES \
  CONFIG_SNODEC_RETRY_BASE \
  CONFIG_SNODEC_RETRY_JITTER \
  CONFIG_SNODEC_RETRY_LIMIT \
  CONFIG_SNODEC_INV4_REUSE_ADDRESS \
  CONFIG_SNODEC_INV4_REUSE_PORT \
  CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_TRUE \
  CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_FALSE \
  CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_DEFAULT \
  CONFIG_SNODEC_IPV4_NUMERIC \
  CONFIG_SNODEC_IPV4_NUMERIC_REVERSE \
  CONFIG_SNODEC_IN6_REUSE_ADDRESS \
  CONFIG_SNODEC_IN6_REUSE_PORT \
  CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_TRUE \
  CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_FALSE \
  CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_DEFAULT \
  CONFIG_SNODEC_IPV6_ONLY \
  CONFIG_SNODEC_IPV4_MAPPED \
  CONFIG_SNODEC_IPV6_NUMERIC \
  CONFIG_SNODEC_IPV6_NUMERIC_REVERSE \
  CONFIG_SNODEC_TLS_INIT_TIMEOUT \
  CONFIG_SNODEC_TLS_SHUTDOWN_TIMEOUT \
  CONFIG_SNODEC_HTTP_REQUEST_PIPELINED \
  CONFIG_PACKAGE_snodec-apps

CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

# FetchContent must use OpenWrt's checked download, never the network at configure time.
define Download/spdlog
  FILE:=spdlog-1.17.0.tar.gz
  URL:=https://codeload.github.com/gabime/spdlog/tar.gz/refs/tags
  URL_FILE:=v1.17.0
  HASH:=d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744
endef
$(eval $(call Download,spdlog))

define Build/Prepare
	$(call Build/Prepare/Default)
	$(TAR) -xzf $(DL_DIR)/spdlog-1.17.0.tar.gz -C $(PKG_BUILD_DIR)
endef

ifdef CONFIG_SNODEC_EPOLL
  CONFIG_SNODEC_IO_MULTIPLEXER=epoll
else ifdef CONFIG_SNODEC_POLL
  CONFIG_SNODEC_IO_MULTIPLEXER=poll
else ifdef CONFIG_SNODEC_SELECT
  CONFIG_SNODEC_IO_MULTIPLEXER=select
endif

ifdef CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_TRUE
  CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM=true
else ifdef CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_FALSE
  CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM=false
else
  CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM=default
endif

ifdef CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_TRUE
  CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM=true
else ifdef CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_FALSE
  CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM=false
else
  CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM=default
endif

# Reset upstream's derived cache so changed OpenWrt settings take effect.
CMAKE_OPTIONS+=-U'SNODEC_*' \
               -DCMAKE_CXX_STANDARD_LIBRARIES=-latomic \
               -DFETCHCONTENT_SOURCE_DIR_SPDLOG=$(PKG_BUILD_DIR)/spdlog-1.17.0 \
               -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
               -DSPDLOG_INSTALL=OFF -DSPDLOG_SYSTEM_INCLUDES=ON \
               -DSNODEC_BUILD_APPS=$(if $(CONFIG_PACKAGE_snodec-apps),ON,OFF) \
               -DSNODEC_BUILD_TESTS=OFF \
               -DSNODEC_CONTROL_BUILD_TUI=OFF \
               -DGROUP_NAME=$(patsubst "%",%,$(CONFIG_SNODEC_GROUP_NAME)) \
               -DIO_MULTIPLEXER=$(CONFIG_SNODEC_IO_MULTIPLEXER) \
               -DREAD_BLOCKSIZE=$(CONFIG_SNODEC_READ_BLOCKSIZE) \
               -DWRITE_BLOCKSIZE=$(CONFIG_SNODEC_WRITE_BLOCKSIZE) \
               -DREAD_TIMEOUT=$(CONFIG_SNODEC_READ_TIMEOUT) \
               -DMAXIMUM_WRITE_QUEUE_BYTES=$(CONFIG_SNODEC_MAXIMUM_WRITE_QUEUE_BYTES) \
               -DWRITE_QUEUE_HIGH_WATERMARK=$(CONFIG_SNODEC_WRITE_QUEUE_HIGH_WATERMARK) \
               -DWRITE_QUEUE_LOW_WATERMARK=$(CONFIG_SNODEC_WRITE_QUEUE_LOW_WATERMARK) \
               -DWRITE_TIMEOUT=$(CONFIG_SNODEC_WRITE_TIMEOUT) \
               -DTERMINATE_TIMEOUT=$(CONFIG_SNODEC_TERMINATE_TIMEOUT) \
               -DBACKLOG=$(CONFIG_SNODEC_BACKLOG) \
               -DACCEPTS_PER_TICK=$(CONFIG_SNODEC_ACCEPTS_PER_TICK) \
               -DIN_REUSE_ADDRESS=$(CONFIG_SNODEC_INV4_REUSE_ADDRESS) \
               -DIN_REUSE_PORT=$(CONFIG_SNODEC_INV4_REUSE_PORT) \
               -DIN_SERVER_DISABLE_NAGLE_ALGORITHM=$(CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM) \
               -DIN_CLIENT_DISABLE_NAGLE_ALGORITHM=$(CONFIG_SNODEC_INV4_DISABLE_NAGLE_ALGORITHM) \
               -DIN_NUMERIC=$(CONFIG_SNODEC_IPV4_NUMERIC) \
               -DIN_NUMERIC_REVERSE=$(CONFIG_SNODEC_IPV4_NUMERIC_REVERSE) \
               -DIN6_REUSE_ADDRESS=$(CONFIG_SNODEC_IN6_REUSE_ADDRESS) \
               -DIN6_REUSE_PORT=$(CONFIG_SNODEC_IN6_REUSE_PORT) \
               -DIN6_SERVER_DISABLE_NAGLE_ALGORITHM=$(CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM) \
               -DIN6_CLIENT_DISABLE_NAGLE_ALGORITHM=$(CONFIG_SNODEC_INV6_DISABLE_NAGLE_ALGORITHM) \
               -DIN6_IPV6_ONLY=$(if $(CONFIG_SNODEC_IPV6_ONLY),true,false) \
               -DIN6_IPV4_MAPPED=$(CONFIG_SNODEC_IPV4_MAPPED) \
               -DIN6_NUMERIC=$(CONFIG_SNODEC_IPV6_NUMERIC) \
               -DIN6_NUMERIC_REVERSE=$(CONFIG_SNODEC_IPV6_NUMERIC_REVERSE) \
               -DACCEPT_TIMEOUT=$(CONFIG_SNODEC_ACCEPT_TIMEOUT) \
               -DCONNECT_TIMEOUT=$(CONFIG_SNODEC_CONNECT_TIMEOUT) \
               -DRECONNECT=$(CONFIG_SNODEC_RECONNECT) \
               -DRECONNECT_TIME=$(CONFIG_SNODEC_RECONNECT_TIME) \
               -DRETRY=$(CONFIG_SNODEC_RETRY) \
               -DRETRY_ON_FATAL=$(CONFIG_SNODEC_RETRY_ON_FATAL) \
               -DRETRY_TIMEOUT=$(CONFIG_SNODEC_RETRY_TIMEOUT) \
               -DRETRY_TRIES=$(CONFIG_SNODEC_RETRY_TRIES) \
               -DRETRY_BASE=$(CONFIG_SNODEC_RETRY_BASE) \
               -DRETRY_JITTER=$(CONFIG_SNODEC_RETRY_JITTER) \
               -DRETRY_LIMIT=$(CONFIG_SNODEC_RETRY_LIMIT) \
               -DTLS_INIT_TIMEOUT=$(CONFIG_SNODEC_TLS_INIT_TIMEOUT) \
               -DTLS_SHUTDOWN_TIMEOUT=$(CONFIG_SNODEC_TLS_SHUTDOWN_TIMEOUT) \
               -DHTTP_REQUEST_PIPELINED=$(CONFIG_SNODEC_HTTP_REQUEST_PIPELINED) \
               -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
               -DCMAKE_SKIP_RPATH=FALSE \
               -DCHECK_INCLUDES=OFF

define Package/snodec/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=SNode.C
endef

define Package/snodec-common
  $(call Package/snodec/Default)
  TITLE:=Easy to use lightwight layer based network framework for C++
  DEPENDS:=+libstdcpp
  USERID:=$(patsubst "%",%,$(CONFIG_SNODEC_GROUP_NAME)):$(patsubst "%",%,$(CONFIG_SNODEC_GROUP_NAME))
endef

define Package/snodec-common/description
  SNode.C is a very simple to use lightweight highly extendable event
driven layer-based framework for network applications in the spirit
of node.js written entirely in C++.

endef

define Package/snodec-common/config
  source "$(SOURCE)/Config.in"
endef

define Package/snodec-common/conffiles
/etc/snode.c/
endef

define Package/snodec-common/install
	$(INSTALL_DIR) $(1)/etc/snode.c
endef

SO_VERSION=$(shell echo $(PKG_VERSION) | cut -d'.' -f1)

define BuildSNodeCModule
  define Package/snodec-$(1)
    $$(call Package/snodec/Default)
    TITLE:=$(1) module
    DEPENDS:=$(2)
  endef

  define Package/snodec-$(1)/description
    This package contains the module '$(1)' of SNode.C

  endef
  
  define Package/snodec-$(1)/install
	$(INSTALL_DIR) $$(1)$(3)
	$(CP) $(PKG_INSTALL_DIR)$(3)/libsnodec-$(1).so.$(SO_VERSION) $$(1)$(3)/
	$(CP) $(PKG_INSTALL_DIR)$(3)/libsnodec-$(1).so.$(PKG_VERSION) $$(1)$(3)/
  endef

  $$(eval $$(call BuildPackage,snodec-$(1)))
endef

define Package/snodec
  $(call Package/snodec/Default)
  TITLE:=All SNode.C runtime modules, applications and control tool
  DEPENDS:=+snodec-apps +snodec-control +snodec-core-mux-epoll \
           +snodec-core-mux-poll \
           +snodec-core-mux-select \
           +snodec-net-in-stream-legacy \
           +snodec-net-in-stream-tls \
           +snodec-net-in6-stream-legacy \
           +snodec-net-in6-stream-tls \
           +snodec-net-l2-stream-legacy \
           +snodec-net-l2-stream-tls \
           +snodec-net-rc-stream-legacy \
           +snodec-net-rc-stream-tls \
           +snodec-net-un-stream-legacy \
           +snodec-net-un-stream-tls \
           +snodec-net-un-dgram \
           +snodec-http-server-express \
           +snodec-http-server-express-legacy-in \
           +snodec-http-server-express-legacy-in6 \
           +snodec-http-server-express-legacy-rc \
           +snodec-http-server-express-legacy-un \
           +snodec-http-server-express-tls-in \
           +snodec-http-server-express-tls-in6 \
           +snodec-http-server-express-tls-rc \
           +snodec-http-server-express-tls-un \
           +snodec-http-client \
           +snodec-websocket-server \
           +snodec-websocket-client \
           +snodec-mqtt-server-websocket \
           +snodec-mqtt-client-websocket \
           +snodec-db-mariadb
endef

define Package/snodec/description
 This meta-package selects all SNode.C runtime modules, demonstration
applications and the control tool.

endef

define Package/snodec/install
	true
endef

define Package/snodec-apps
  $(call Package/snodec/Default)
  TITLE:=SNode.C demo applications
  DEPENDS:=+snodec-net-in-stream-legacy \
           +snodec-net-in-stream-tls \
           +snodec-net-in6-stream-legacy \
           +snodec-net-in6-stream-tls \
           +snodec-net-l2-stream-legacy \
           +snodec-net-l2-stream-tls \
           +snodec-net-rc-stream-legacy \
           +snodec-net-rc-stream-tls \
           +snodec-net-un-stream-legacy \
           +snodec-net-un-stream-tls \
           +snodec-http-server-express \
           +snodec-http-client \
           +snodec-websocket-server \
           +snodec-websocket-client \
           +snodec-mqtt-server \
           +snodec-mqtt-client \
           +snodec-db-mariadb
endef

define Package/snodec-apps/description
 This package contains all demo applications of SNode.C.

endef

define Package/snodec-apps/install
	$(INSTALL_DIR) $(1)/usr/bin
	find $(PKG_INSTALL_DIR)/usr/bin -mindepth 1 -maxdepth 1 ! -name snodec-control -exec $(CP) {} $(1)/usr/bin/ \;
	$(INSTALL_DIR) $(1)/usr/lib/snode.c/web/http/upgrade/websocket
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/snode.c/web/http/upgrade/websocket/libsnodec-websocket-echo-server.so.$(SO_VERSION) $(1)/usr/lib/snode.c/web/http/upgrade/websocket/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/snode.c/web/http/upgrade/websocket/libsnodec-websocket-echo-server.so.$(PKG_VERSION) $(1)/usr/lib/snode.c/web/http/upgrade/websocket/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/snode.c/web/http/upgrade/websocket/libsnodec-websocket-echo-client.so.$(SO_VERSION) $(1)/usr/lib/snode.c/web/http/upgrade/websocket/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/snode.c/web/http/upgrade/websocket/libsnodec-websocket-echo-client.so.$(PKG_VERSION) $(1)/usr/lib/snode.c/web/http/upgrade/websocket/
endef

define Package/snodec-control
  $(call Package/snodec/Default)
  TITLE:=SNode.C control socket client
  DEPENDS:=+snodec-common
endef

define Package/snodec-control/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/snodec-control $(1)/usr/bin/
endef

$(eval $(call BuildPackage,snodec-common))
$(eval $(call BuildPackage,snodec-apps))
$(eval $(call BuildPackage,snodec-control))
$(eval $(call BuildPackage,snodec))
$(eval $(call BuildSNodeCModule,logger,+snodec-common,/usr/lib))
$(eval $(call BuildSNodeCModule,utils,+snodec-logger,/usr/lib))
$(eval $(call BuildSNodeCModule,core-mux-epoll,+snodec-logger,/usr/lib))
$(eval $(call BuildSNodeCModule,core-mux-poll,+snodec-logger,/usr/lib))
$(eval $(call BuildSNodeCModule,core-mux-select,+snodec-logger,/usr/lib))
$(eval $(call BuildSNodeCModule,core,+snodec-utils +libatomic +SNODEC_EPOLL:snodec-core-mux-epoll +SNODEC_POLL:snodec-core-mux-poll +SNODEC_SELECT:snodec-core-mux-select,/usr/lib))
$(eval $(call BuildSNodeCModule,core-socket,+snodec-core,/usr/lib))
$(eval $(call BuildSNodeCModule,core-socket-stream,+snodec-core-socket,/usr/lib))
$(eval $(call BuildSNodeCModule,core-socket-stream-legacy,+snodec-core-socket-stream,/usr/lib))
$(eval $(call BuildSNodeCModule,core-socket-stream-tls,+snodec-core-socket-stream +libopenssl,/usr/lib))
$(eval $(call BuildSNodeCModule,db-mariadb,+snodec-core +libmariadb,/usr/lib))
$(eval $(call BuildSNodeCModule,net,+snodec-core-socket,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in,+snodec-net,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in-phy,+snodec-net-in,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in-phy-stream,+snodec-net-in-phy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in-stream,+snodec-net-in-phy-stream,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in-stream-legacy,+snodec-net-in-stream +snodec-core-socket-stream-legacy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in-stream-tls,+snodec-net-in-stream +snodec-core-socket-stream-tls,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in6,+snodec-net,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in6-phy,+snodec-net-in6,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in6-phy-stream,+snodec-net-in6-phy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in6-stream,+snodec-net-in6-phy-stream,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in6-stream-legacy,+snodec-net-in6-stream +snodec-core-socket-stream-legacy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-in6-stream-tls,+snodec-net-in6-stream +snodec-core-socket-stream-tls,/usr/lib))
$(eval $(call BuildSNodeCModule,net-l2,+snodec-net +bluez-libs,/usr/lib))
$(eval $(call BuildSNodeCModule,net-l2-phy,+snodec-net-l2 +bluez-libs,/usr/lib))
$(eval $(call BuildSNodeCModule,net-l2-phy-stream,+snodec-net-l2-phy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-l2-stream,+snodec-net-l2-phy-stream,/usr/lib))
$(eval $(call BuildSNodeCModule,net-l2-stream-legacy,+snodec-net-l2-stream +snodec-core-socket-stream-legacy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-l2-stream-tls,+snodec-net-l2-stream +snodec-core-socket-stream-tls,/usr/lib))
$(eval $(call BuildSNodeCModule,net-rc,+snodec-net +bluez-libs,/usr/lib))
$(eval $(call BuildSNodeCModule,net-rc-phy,+snodec-net-rc +bluez-libs,/usr/lib))
$(eval $(call BuildSNodeCModule,net-rc-phy-stream,+snodec-net-rc-phy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-rc-stream,+snodec-net-rc-phy-stream,/usr/lib))
$(eval $(call BuildSNodeCModule,net-rc-stream-legacy,+snodec-net-rc-stream +snodec-core-socket-stream-legacy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-rc-stream-tls,+snodec-net-rc-stream +snodec-core-socket-stream-tls,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un,+snodec-net,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un-phy,+snodec-net-un,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un-phy-stream,+snodec-net-un-phy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un-stream,+snodec-net-un-phy-stream,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un-stream-legacy,+snodec-net-un-stream +snodec-core-socket-stream-legacy,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un-stream-tls,+snodec-net-un-stream +snodec-core-socket-stream-tls,/usr/lib))
$(eval $(call BuildSNodeCModule,net-un-dgram,+snodec-net-un-phy +snodec-net-un,/usr/lib))
$(eval $(call BuildSNodeCModule,http,+snodec-core-socket-stream +libmagic,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server,+snodec-http,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-client,+snodec-http,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express,+snodec-http-server,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-legacy-in,+snodec-http-server-express +snodec-net-in-stream-legacy,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-legacy-in6,+snodec-http-server-express +snodec-net-in6-stream-legacy,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-legacy-rc,+snodec-http-server-express +snodec-net-rc-stream-legacy,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-legacy-un,+snodec-http-server-express +snodec-net-un-stream-legacy,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-tls-in,+snodec-http-server +snodec-http-server-express +snodec-net-in-stream-tls,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-tls-in6,+snodec-http-server +snodec-http-server-express +snodec-net-in6-stream-tls,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-tls-rc,+snodec-http-server +snodec-http-server-express +snodec-net-rc-stream-tls,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,http-server-express-tls-un,+snodec-http-server +snodec-http-server-express +snodec-net-un-stream-tls,/usr/lib/snode.c/web/http))
$(eval $(call BuildSNodeCModule,websocket,+snodec-utils,/usr/lib/snode.c/web/http/upgrade))
$(eval $(call BuildSNodeCModule,websocket-server,+snodec-websocket +snodec-http-server,/usr/lib/snode.c/web/http/upgrade))
$(eval $(call BuildSNodeCModule,websocket-client,+snodec-websocket +snodec-http-client,/usr/lib/snode.c/web/http/upgrade))
$(eval $(call BuildSNodeCModule,mqtt,+snodec-core-socket-stream,/usr/lib/snode.c/iot/mqtt))
$(eval $(call BuildSNodeCModule,mqtt-server,+snodec-mqtt,/usr/lib/snode.c/iot/mqtt))
$(eval $(call BuildSNodeCModule,mqtt-client,+snodec-mqtt,/usr/lib/snode.c/iot/mqtt))
$(eval $(call BuildSNodeCModule,mqtt-server-websocket,+snodec-mqtt-server +snodec-websocket-server,/usr/lib/snode.c/iot/mqtt))
$(eval $(call BuildSNodeCModule,mqtt-client-websocket,+snodec-mqtt-client +snodec-websocket-client,/usr/lib/snode.c/iot/mqtt))
