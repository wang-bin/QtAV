#include <ImageConverter.h>

namespace QtAV {

class gcScalerPrivate;
class gcScaler Q_DECL_FINAL : public ImageConverter
{
public:
    bool check() const Q_DECL_OVERRIDE;
    // FIXME: why match to the pure virtual one if not declare here?
    //bool convert(const quint8 *const src[], const int srcStride[]) Q_DECL_OVERRIDE { return ImageConverter::convert(src, srcStride);}
    bool convert(const quint8 *const src[], const int srcStride[], quint8 *const dst[], const int dstStride[]) Q_DECL_OVERRIDE;
protected:
    bool prepareData() Q_DECL_OVERRIDE;
};
typedef gcScaler ImageConverterVIV;
} //namespace QtAV
