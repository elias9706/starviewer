#include "autotest.h"
#include "imageoverlayreader.h"

#include <gdcmImage.h>

using namespace udg;

class TestingImageOverlayReader : public ImageOverlayReader {
public:
    gdcm::Image *m_gdcmImage;

private:
    virtual gdcm::Image *getGDCMImageFromFile(const QString &filename)
    {
        Q_UNUSED(filename);
        return m_gdcmImage;
    }
};

Q_DECLARE_METATYPE(gdcm::Image*);
Q_DECLARE_METATYPE(QList<ImageOverlay>);

class test_ImageOverlayReader : public QObject {
Q_OBJECT

private slots:
    void read_ShouldReturnExpectedValue_data();
    void read_ShouldReturnExpectedValue();
    
    void getOverlays_ShouldReturnEmptyList_data();
    void getOverlays_ShouldReturnEmptyList();

    void getOverlays_ShouldReturnExpectedOverlays_data();
    void getOverlays_ShouldReturnExpectedOverlays();

private:
    /// Ens converteix un gdcm::Overlay a ImageOverlay
    ImageOverlay gdcmOverlayToImageOverlay(const gdcm::Overlay &gdcmOverlay);

    /// Compara si dos ImageOverlay s�n iguals o no
    bool areEqual(const ImageOverlay &overlay1, const ImageOverlay &overlay2);
};

void test_ImageOverlayReader::read_ShouldReturnExpectedValue_data()
{
    QTest::addColumn<gdcm::Image*>("gdcmImage");
    QTest::addColumn<bool>("returnValue");

    gdcm::Image *nullImage = 0;
    QTest::newRow("Could not read file (Null image)") << nullImage << false;

    gdcm::Image *imageWithoutOverlays = new gdcm::Image;
    QTest::newRow("Image without overlays") << imageWithoutOverlays << true;

    gdcm::Image *imageWithOneOverlay = new gdcm::Image;
    imageWithOneOverlay->SetNumberOfOverlays(1);
    QTest::newRow("Image with 1 overlay") << imageWithOneOverlay << true;

    gdcm::Image *imageWithSixteenOverlays = new gdcm::Image;
    imageWithSixteenOverlays->SetNumberOfOverlays(16);
    QTest::newRow("Image with 16 overlays") << imageWithSixteenOverlays << true;

    gdcm::Image *imageWithTwentyFiveOverlays = new gdcm::Image;
    imageWithTwentyFiveOverlays->SetNumberOfOverlays(25);
    QTest::newRow("Image with 25 overlays") << imageWithTwentyFiveOverlays << true;
}

void test_ImageOverlayReader::read_ShouldReturnExpectedValue()
{
    QFETCH(gdcm::Image*, gdcmImage);
    QFETCH(bool, returnValue);

    TestingImageOverlayReader overlayReader;
    overlayReader.m_gdcmImage = gdcmImage;

    QCOMPARE(overlayReader.read(), returnValue);
}

void test_ImageOverlayReader::getOverlays_ShouldReturnEmptyList_data()
{
    QTest::addColumn<gdcm::Image*>("gdcmImage");

    gdcm::Image *nullImagePointer = 0;
    QTest::newRow("no image read (image read fail)") << nullImagePointer;
    
    gdcm::Image *image1 = new gdcm::Image;
    QTest::newRow("image without overlays") << image1;
    
    gdcm::Image *image2 = new gdcm::Image;
    image2->SetNumberOfOverlays(0);
    QTest::newRow("gdcmImage.setNumberOfOverlays(0)") << image2;
}

void test_ImageOverlayReader::getOverlays_ShouldReturnEmptyList()
{
    QFETCH(gdcm::Image*, gdcmImage);

    TestingImageOverlayReader overlayReader;
    overlayReader.m_gdcmImage = gdcmImage;
    overlayReader.read();

    QVERIFY(overlayReader.getOverlays().isEmpty());
}

void test_ImageOverlayReader::getOverlays_ShouldReturnExpectedOverlays_data()
{
    QTest::addColumn<gdcm::Image*>("gdcmImage");
    QTest::addColumn<QList<ImageOverlay> >("overlaysList");

    gdcm::Image *image = new gdcm::Image;
    image->SetNumberOfOverlays(1);

    QList<ImageOverlay> overlaysList;
    overlaysList << gdcmOverlayToImageOverlay(image->GetOverlay(0));
    QTest::newRow("Image with 1 empty overlay") << image << overlaysList;

    gdcm::Image *image2 = new gdcm::Image;
    image2->SetNumberOfOverlays(1);
    gdcm::Overlay &overlay = image2->GetOverlay(0);
    overlay.SetColumns(10);
    overlay.SetRows(10);
    short origin[2] = {1, 3};
    overlay.SetOrigin((const short*)origin);
    const char *overlayData = new char[100];
    overlay.SetOverlay(overlayData, 100);

    QList<ImageOverlay> overlaysList1;
    overlaysList1 << gdcmOverlayToImageOverlay(overlay);
    QTest::newRow("Image with 1 overlay (with data)") << image2 << overlaysList1;
}

void test_ImageOverlayReader::getOverlays_ShouldReturnExpectedOverlays()
{
    QFETCH(gdcm::Image*, gdcmImage);
    QFETCH(QList<ImageOverlay>, overlaysList);

    TestingImageOverlayReader overlayReader;
    overlayReader.m_gdcmImage = gdcmImage;
    overlayReader.read();

    int numberOfOverlaysRead = overlayReader.getOverlays().count();
    QCOMPARE(numberOfOverlaysRead, overlaysList.count());

    QList<ImageOverlay> listOfReadOverlays = overlayReader.getOverlays();
    for (int i = 0; i < numberOfOverlaysRead; ++i)
    {
        QVERIFY(areEqual(listOfReadOverlays.at(i), overlaysList.at(i)));
    }
}

ImageOverlay test_ImageOverlayReader::gdcmOverlayToImageOverlay(const gdcm::Overlay &gdcmOverlay)
{
    ImageOverlay imageOverlay;
    imageOverlay.setColumns(gdcmOverlay.GetColumns());
    imageOverlay.setRows(gdcmOverlay.GetRows());
    const signed short *origin = gdcmOverlay.GetOrigin();
    imageOverlay.setOrigin(static_cast<int>(origin[0]), static_cast<int>(origin[1]));
    
    unsigned char *buffer = new unsigned char[imageOverlay.getRows() * imageOverlay.getColumns()];
    gdcmOverlay.GetUnpackBuffer(buffer);
    imageOverlay.setData(buffer);

    return imageOverlay;
}

bool test_ImageOverlayReader::areEqual(const ImageOverlay &overlay1, const ImageOverlay &overlay2)
{
    bool equal = overlay1.getColumns() == overlay2.getColumns() && overlay1.getRows() == overlay2.getRows() && overlay1.getXOrigin() == overlay2.getXOrigin()
        && overlay1.getYOrigin() == overlay2.getYOrigin();

    if (!equal)
    {
        return false;
    }
    
    // Tots els atributs s�n iguals, comparem els buffers
    unsigned char *buffer1 = overlay1.getData();
    unsigned char *buffer2 = overlay2.getData();
    
    if (buffer1 && buffer2)
    {
        int length = overlay1.getColumns() * overlay1.getRows();
        int index = 0;
        while (length-- > 0)
        {
            if (buffer1[index] == buffer2[index])
            {
                ++index;
            }
            else
            {
                return false;
            }
        }
        return true;
    }
    else if (!buffer1 && !buffer2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

DECLARE_TEST(test_ImageOverlayReader)

#include "test_imageoverlayreader.moc"