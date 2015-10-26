#include <ImageConverter.h>

namespace QtAV {
class GALScalerPrivate;
class GALScaler Q_DECL_FINAL : public ImageConverter
{
public:
    GALScaler();
    bool check() const Q_DECL_OVERRIDE;
    // FIXME: why match to the pure virtual one if not declare here?
    // TODO: move to private
    bool convert(const quint8 *const src[], const int srcStride[]) Q_DECL_OVERRIDE;
    // dst is allocated by gpu. if convert to given dst(host), use dma copy
    bool convert(const quint8 *const src[], const int srcStride[], quint8 *const dst[], const int dstStride[]) Q_DECL_OVERRIDE;
protected:
    bool prepareData() Q_DECL_OVERRIDE;
};
typedef GALScaler ImageConverterVIV;
} //namespace QtAV
