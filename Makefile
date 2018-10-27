# $FreeBSD$

SUBDIR=	linuxkpi	\
	lindebugfs	\
	vmwgfx		\
	drm		\
	i915		\
	amd		\
	radeon		\
	${_staging}	


.if defined(STAGING)
_staging=	staging
.endif

.include <bsd.subdir.mk>
