#include <QImage>
#include "Platform.h"
#include "Material.h"
#include "RenderState.h"
#include "dds.h"
#include "dxt.h"

Material::Material()
{
    m_ambient = vec4(0.0, 0.0, 0.0, 0.0);
    m_diffuse = vec4(0.0, 0.0, 0.0, 0.0);
    m_specular = vec4(0.0, 0.0, 0.0, 0.0);
    m_shine = 0.0;
    m_origin = LowerLeft;
    m_texture = 0;
    m_subTexture = 0;
    m_opaque = true;
}

Material::Material(vec4 ambient, vec4 diffuse, vec4 specular, float shine)
{
    m_ambient = ambient;
    m_diffuse = diffuse;
    m_specular = specular;
    m_shine = shine;
    m_origin = LowerLeft;
    m_texture = 0;
    m_opaque = true;
}

const vec4 & Material::ambient() const
{
    return m_ambient;
}

const vec4 & Material::diffuse() const
{
    return m_diffuse;
}

const vec4 & Material::specular() const
{
    return m_specular;
}

float Material::shine() const
{
    return m_shine;
}

void Material::setAmbient(const vec4 &ambient)
{
    m_ambient = ambient;
}

void Material::setDiffuse(const vec4 &diffuse)
{
    m_diffuse = diffuse;
}

void Material::setSpecular(const vec4 &specular)
{
    m_specular = specular;
}

void Material::setShine(float shine)
{
    m_shine = shine;
}

bool Material::isOpaque() const
{
    return m_opaque;
}

void Material::setOpaque(bool opaque)
{
    m_opaque = opaque;
}

QImage Material::image() const
{
    return m_img;
}

void Material::setImage(QImage newImage)
{
    m_img = newImage;
}

Material::OriginType Material::origin() const
{
    return m_origin;
}

void Material::setOrigin(Material::OriginType newOrigin)
{
    m_origin = newOrigin;
}

texture_t Material::texture() const
{
    return m_texture;
}

void Material::setTexture(texture_t texture)
{
    m_texture = texture;
}

uint Material::subTexture() const
{
    return m_subTexture;
}

void Material::setSubTexture(uint newID)
{
    m_subTexture = newID;
}

bool Material::loadTextureDDS(const char *data, size_t size, QImage &img)
{
    dds_header_t hdr;
    dds_pixel_format_t pf;
    const char *d = data;
    size_t left = size;
    if(left < sizeof(hdr))
        return false;
    memcpy(&hdr, d, sizeof(hdr));
    left -= sizeof(hdr);
    d += sizeof(hdr);

    if(memcmp(hdr.magic, "DDS ", 4) || (hdr.size != 124) ||
        !(hdr.flags & DDSD_PIXELFORMAT) || !(hdr.flags & DDSD_CAPS) )
        return false;
    if(!(hdr.flags & DDSD_LINEARSIZE) || (hdr.pitch_or_linsize > left))
        return false;

    pf = hdr.pixelfmt;
    if((pf.flags & DDPF_FOURCC))
    {
        int format = DDS_COMPRESS_NONE;
        if(memcmp(pf.fourcc, "DXT1", 4) == 0)
            format = DDS_COMPRESS_BC1;
        else if(memcmp(pf.fourcc, "DXT5", 4) == 0)
            format = DDS_COMPRESS_BC3;
        if(format != DDS_COMPRESS_NONE)
        {
            img = QImage(hdr.width, hdr.height, QImage::Format_ARGB32);
            dxt_decompress(img.bits(), (unsigned char *)d, format, left, hdr.width, hdr.height, 4);
            return true;
        }
    }
    qDebug("DDS format not supported / not implemented");
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MaterialMap::MaterialMap()
{
    m_uploaded = false;
    m_arrayTexture = 0;
}

MaterialMap::~MaterialMap()
{
    foreach(Material *mat, m_materials)
        delete mat;
}

const QMap<QString, Material *> & MaterialMap::materials() const
{
    return m_materials;
}

Material * MaterialMap::material(QString name) const
{
    return m_materials.value(name);
}

void MaterialMap::setMaterial(QString name, Material *mat)
{
    m_materials[name] = mat;
}

texture_t MaterialMap::arrayTexture() const
{
    return m_arrayTexture;
}

bool MaterialMap::uploaded() const
{
    return m_uploaded;
}

void MaterialMap::textureArrayInfo(int &maxWidth, int &maxHeight,
                                   size_t &totalMem, size_t &usedMem) const
{
    const size_t pixelSize = 4;
    size_t count = 0;
    maxWidth = maxHeight = totalMem = usedMem = 0;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        int width = mat->image().width();
        int height = mat->image().height();
        maxWidth = qMax(maxWidth, width);
        maxHeight = qMax(maxHeight, height);
        usedMem += (width * height * pixelSize);
        //qDebug("Texture %d x %d", width, height);
        count++;
    }
    totalMem = maxWidth * maxHeight * pixelSize * count;
}

void MaterialMap::upload(RenderState *state)
{
    if(m_uploaded)
        return;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        bool convertToGL = mat->origin() != Material::LowerLeft;
        if(!mat->image().isNull() && (mat->texture() == 0))
        {
            mat->setTexture(state->loadTexture(mat->image(), convertToGL));
            mat->image() = QImage();
        }
    }
    m_uploaded = true;
}

void MaterialMap::uploadArray(RenderState *state)
{
    if(m_uploaded)
        return;
    QVector<Material *> uploadMats;
    QVector<QImage> images;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        if(!mat->image().isNull() && (mat->texture() == 0))
        {
            uploadMats.append(mat);
            images.append(mat->image());
        }
    }
    
    m_arrayTexture = state->loadTextures(images.constData(), images.count(), true);
    for(uint i = 0; i < uploadMats.count(); i++)
    {
        Material *mat = uploadMats[i];
        mat->setTexture(m_arrayTexture);
        mat->setSubTexture(i);
        mat->setImage(QImage());
    }
    m_uploaded = true;
}
