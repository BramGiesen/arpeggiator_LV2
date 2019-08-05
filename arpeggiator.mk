######################################
#
# arpeggiator
#
######################################

# where to find the source code - locally in this case
ARPEGGIATOR_SITE_METHOD = local
ARPEGGIATOR_SITE = $($(PKG)_PKGDIR)/

# even though this is a local build, we still need a version number
# bump this number if you need to force a rebuild
ARPEGGIATOR_VERSION = 1

# dependencies (list of other buildroot packages, separated by space)
ARPEGGIATOR_DEPENDENCIES =

# LV2 bundles that this package generates (space separated list)
ARPEGGIATOR_BUNDLES = arpeggiator.lv2

# call make with the current arguments and path. "$(@D)" is the build directory.
ARPEGGIATOR_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/source


# build command
define ARPEGGIATOR_BUILD_CMDS
	$(ARPEGGIATOR_TARGET_MAKE)
endef

# install command
define ARPEGGIATOR_INSTALL_TARGET_CMDS
	$(ARPEGGIATOR_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef


# import everything else from the buildroot generic package
$(eval $(generic-package))
