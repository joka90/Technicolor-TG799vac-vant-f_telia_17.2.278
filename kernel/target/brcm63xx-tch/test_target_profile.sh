RELEASE=4.16L.04

	m4 \
		  -D_CHIPSET=63138\
		  -D_IQCTL=n \
		  -D_SNOOPCTL=n \
		  -D_RDPACTL=n \
		  -D_FAPCTL=n \
		  -D_GMACCTL=n \
		  -D_FAPCTL=n \
		  -D_BRCM_DHD=n \
		  -D_SWMDK=y \
          -D_VOICE=n \
		  -D_BUILD_SPUCTL=n \
		  -D_SELT=n \
		  -D_WFD=y \
		  -D_VOICE=y \
		  target_profile.$RELEASE > profile.63138

	m4 \
		  -D_CHIPSET=63381\
		  -D_IQCTL=y \
		  -D_SNOOPCTL=n \
		  -D_RDPACTL=n \
		  -D_FAPCTL=n \
		  -D_GMACCTL=n \
		  -D_SWMDK=y \
		  -D_BRCM_DHD=n \
          -D_VOICE=n \
		  -D_BUILD_SPUCTL=n \
		  -D_SELT=n \
		  -D_WFD=y \
		  target_profile.$RELEASE > profile.63381

	m4 \
		  -D_CHIPSET=63268\
		  -D_IQCTL=y \
		  -D_SNOOPCTL=n \
		  -D_RDPACTL=n \
		  -D_FAP=y \
		  -D_FAPCTL=y \
		  -D_GMACCTL=y \
		  -D_ISDN=y \
		  -D_BRCM_DHD=n \
		  -D_VOICE=n \
		  -D_SWMDK=y \
		  -D_BUILD_SPUCTL=n \
		  -D_SELT=n \
		  -D_WFD=y \
		  -D_VOICE=y \
		  target_profile.$RELEASE > profile.63268

	m4 \
		  -D_CHIPSET=6318\
		  -D_IQCTL=y \
		  -D_SNOOPCTL=n \
		  -D_RDPACTL=n \
		  -D_FAPCTL=n \
		  -D_GMACCTL=n \
		  -D_SWMDK=n \
		  -D_BRCM_DHD=n \
		  -D_VOICE=n \
		  -D_BUILD_SPUCTL=n \
		  -D_SELT=n \
		  -D_WFD=n \
		  target_profile.$RELEASE > profile.6318
