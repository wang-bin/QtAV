#include <ImageConverter.h>

namespace QtAV {
class GALScalerPrivate;
class GALScaler Q_DECL_FINAL : public ImageConverter
{
public:
    GALScaler();
    bool check() const Q_DECL_OVERRIDE;
    // dst is allocated by gpu. if convert to given dst(host), use dma copy
    bool convert(const quint8 *const src[], const int srcStride[], quint8 *const dst[], const int dstStride[]) Q_DECL_OVERRIDE;
protected:
    // we lock the out surface in convert() so the physical address is not accessible outside. so it can only be used internally
    bool convert(const quint8 *const src[], const int srcStride[]) Q_DECL_OVERRIDE;
    bool prepareData() Q_DECL_OVERRIDE;
};
typedef GALScaler ImageConverterVIV;
} //namespace QtAV
