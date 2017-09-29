//---------------------------------------------------------------------------
#ifndef RESAMPLING_HPP
#define RESAMPLING_HPP
#include "image/utility/basic_image.hpp"
#include "image/numerical/transformation.hpp"
#include "image/numerical/numerical.hpp"
#include "image/numerical/basic_op.hpp"
#include "image/numerical/statistics.hpp"
#include "interpolation.hpp"

namespace image
{

template<class IntegerType>
class PixelAdapter
{
private:
    unsigned int r_16;
public:
    PixelAdapter(void):r_16(0) {}
    void add(IntegerType gray,unsigned int w_16)
    {
        r_16 += ((unsigned int)gray)*w_16;
    }
    void add(IntegerType gray,const unsigned int* w)
    {
        r_16 += w[gray];
    }
    IntegerType get(void) const
    {
        return r_16 >> 16;
    }
};

template<>
class PixelAdapter<rgb_color>
{
private:
    unsigned int r_16,g_16,b_16;
public:
    PixelAdapter<rgb_color>(void):r_16(0),g_16(0),b_16(0) {}
    void add(rgb_color color,unsigned int w_16)
    {
        r_16 += ((unsigned int)color.r)*w_16;
        g_16 += ((unsigned int)color.g)*w_16;
        b_16 += ((unsigned int)color.b)*w_16;
    }
    void add(rgb_color color,const unsigned int* w_16)
    {
        r_16 += w_16[color.r];
        g_16 += w_16[color.g];
        b_16 += w_16[color.b];
    }
    rgb_color get(void) const
    {
        return rgb_color(r_16 >> 16,g_16 >> 16,b_16 >> 16);
    }
};
//---------------------------------------------------------------------------
template<class PixelType>
void thumb(const basic_image<PixelType,2>& from,basic_image<PixelType,2>& to)
{
    std::vector<PixelAdapter<PixelType> > to_buffer(to.width());
    enum MoveType {MoveFrom,MoveTo};

    const unsigned int value_16 = 1 << 16;
    unsigned int height_r_16 = ((double)to.height())/((double)from.height())*((double)((unsigned int)1 << 16));
    unsigned int width_r_16 = ((double)to.width())/((double)from.width())*((double)((unsigned int)1 << 16));
    unsigned int height_r_8 = height_r_16 >> 8;
    unsigned int width_r_8 = width_r_16 >> 8;
    unsigned int rr_16 = ((double)to.size())/((double)from.size())*((double)((unsigned int)1 << 16));

    unsigned int tmp_rr_16;
    bool rr_buffer;

    //std::vector<unsigned int> rbuffer_16(PixelAdapter<PixelType>::max);
    //for (unsigned int index = 0;index < PixelAdapter<PixelType>::max;++index)
    //    rbuffer_16[index] = rr_16*index;

    unsigned int next_from_y_16 = height_r_16;
    unsigned int next_to_y_16 = value_16;
    unsigned int from_y = 0;
    unsigned int to_y = 0;
    unsigned int from_y_index = 0;
    unsigned int to_y_index = 0;
    unsigned int y_16 = 0;
    MoveType y_move = MoveFrom;
    while (1)
    {
        unsigned int y_interval_8;
        if (next_from_y_16 < next_to_y_16)
        {
            y_interval_8 = (y_move == MoveFrom) ? height_r_8 : (next_from_y_16-y_16) >> 8;
            y_move = MoveFrom;
        }
        else
        {
            y_interval_8 = (y_move == MoveTo) ? (1 << 8) : (next_to_y_16-y_16) >> 8;
            y_move = MoveTo;
        }

        if (!(rr_buffer = (y_interval_8 == height_r_8)))
            tmp_rr_16 = width_r_8 * y_interval_8;
        {
            unsigned int next_from_x_16 = width_r_16;
            unsigned int next_to_x_16 = value_16;
            unsigned int from_x = 0;
            unsigned int to_x = 0;
            unsigned int x_16 = 0;
            MoveType x_move = MoveFrom;
            while (1)
            {
                unsigned int w_16;
                if (next_from_x_16 < next_to_x_16)
                {
                    if (x_move == MoveFrom)
                    {
                        if (rr_buffer)
                            to_buffer[to_x].add(from[from_y_index + from_x],rr_16);
                        else
                            to_buffer[to_x].add(from[from_y_index + from_x],tmp_rr_16);
                    }
                    else
                        to_buffer[to_x].add(from[from_y_index + from_x],
                                            ((next_from_x_16-x_16) >> 8)* (y_interval_8));
                    x_move = MoveFrom;
                    x_16 = next_from_x_16;
                    ++from_x;
                    next_from_x_16 += width_r_16;
                    if (from_x >= from.width())
                        break;
                }
                else
                {
                    w_16 = (x_move == MoveTo) ? (y_interval_8 << 8) :
                           ((next_to_x_16-x_16) >> 8) * y_interval_8;
                    x_move = MoveTo;

                    to_buffer[to_x].add(from[from_y_index + from_x],w_16);

                    x_16 = next_to_x_16;
                    ++to_x;
                    next_to_x_16 += value_16;
                    if (to_x >= to.width())
                        break;
                }
            }
        }

        if (y_move == MoveFrom)
        {
            y_16 = next_from_y_16;
            ++from_y;
            from_y_index += from.width();
            next_from_y_16 += height_r_16;
            if (from_y >= from.height())
                break;
        }
        else
        {
            y_16 = next_to_y_16;
            ++to_y;
            next_to_y_16 += value_16;
            if (to_y >= to.height())
                break;
            for (unsigned int index = 0;index < to.width();++index)
                to[to_y_index + index] = to_buffer[index].get();
            to_buffer.resize(0);
            to_buffer.resize(to.width());
            to_y_index += to.width();
        }
    }
    for (unsigned int index = 0;index < to.width();++index)
        to[to_y_index + index] = to_buffer[index].get();

}
//---------------------------------------------------------------------------
template<class PixelType,class ImageType2D>
void reslicing(const basic_image<PixelType,2>& slice,ImageType2D& image,
               unsigned int dim,unsigned int slice_index)
{
    image = slice;
}

template<class ImageType3D,class ImageType2D>
void reslicing(const ImageType3D& slice,ImageType2D& image,
               unsigned int dim,unsigned int slice_index)
{
    const geometry<3>& geo = slice.geometry();
    if (dim == 2)   //XY
    {
        if(slice_index >= slice.depth())
            return;
        image.resize(geometry<2>(geo[0],geo[1]));
        std::copy(slice.begin() + slice_index*image.size(),
                  slice.begin() + (slice_index+1)*image.size(),
                  image.begin());

    }
    else
        if (dim == 1)   //XZ
        {
            if(slice_index >= slice.height())
                return;
            image.resize(geometry<2>(geo[0],geo[2]));
            unsigned int wh = geo[0]*geo[1];
            unsigned int sindex = slice_index*geo[0];
            for (unsigned int index = 0;index < image.size();index += geo[0],sindex += wh)
                std::copy(slice.begin() + sindex,
                          slice.begin() + sindex+geo[0],
                          image.begin() + index);
        }
        else
            if (dim == 0)    //YZ
            {
                if(slice_index >= slice.width())
                    return;
                image.resize(geometry<2>(geo[1],geo[2]));
                unsigned int sindex = slice_index;
                unsigned int w = geo[0];
                for (unsigned int index = 0;index < image.size();++index,sindex += w)
                    image[index] = slice[sindex];
            }

}

template<typename value_type>
struct pixel_average{
    value_type operator()(value_type l,value_type r) const
    {
        return (l+r)/2;
    }
};

template<>
struct pixel_average<short>{
    short operator()(short l,short r) const
    {
        int avg = l;
        avg += r;
        return avg >> 1;
    }
};

template<>
struct pixel_average<int>{
    int operator()(int l,int r) const
    {
        return (l+r) >> 1;
    }
};

template<>
struct pixel_average<float>{
    float operator()(float l,float r) const
    {
        return (l+r)*0.5f;
    }
};
template<>
struct pixel_average<image::rgb_color>{
    image::rgb_color operator()(image::rgb_color c1,image::rgb_color c2) const
    {
        short r = c1.r;
        short g = c1.g;
        short b = c1.b;
        r += c2.r;
        g += c2.g;
        b += c2.b;
        return image::rgb_color(r >> 1,g >> 1,b >> 1);
    }
};

template<class IteratorType,class OutputIterator>
OutputIterator upsampling_x(IteratorType from,IteratorType to,OutputIterator out,int width)
{
    IteratorType line_iter = to;
    out += (to-from) << 1;
    OutputIterator result = out;
    do
    {
        to -= width;
        --line_iter;
        *(--out) = *line_iter;
        *(--out) = *line_iter;
        if(line_iter != to)
        do{
            --line_iter;
            --out;
            *out = pixel_average<typename IteratorType::value_type>()(*line_iter,*(line_iter+1));
            *(--out) = *line_iter;
        }
        while(line_iter != to);

        if(to == from)
            break;
        line_iter = to;
    }
    while(1);
    return result;
}

template<class IteratorType,class OutputIterator>
OutputIterator upsampling_x_nearest(IteratorType from,IteratorType to,OutputIterator out,int width)
{
    IteratorType line_iter = to;
    out += (to-from) << 1;
    OutputIterator result = out;
    do
    {
        to -= width;
        --line_iter;
        *(--out) = *line_iter;
        *(--out) = *line_iter;
        if(line_iter != to)
        do{
            --line_iter;
            *(--out) = *line_iter;
            *(--out) = *line_iter;
        }
        while(line_iter != to);

        if(to == from)
            break;
        line_iter = to;
    }
    while(1);
    return result;
}


template<class IteratorType,class OutputIterator>
OutputIterator upsampling_y(IteratorType from,IteratorType to,OutputIterator out,int width,int height)
{
    int plane_size = width*height;
    IteratorType plane_iter = to;
    IteratorType plane_end;
    out += (to-from) << 1;
    OutputIterator result = out;
    do
    {
        to -= plane_size;

        plane_end = plane_iter;
        plane_iter -= width;

        out -= width;
        std::copy(plane_iter,plane_end,out);
        out -= width;
        std::copy(plane_iter,plane_end,out);

        if(plane_iter != to)
        do{
            out -= width;
            std::copy(plane_iter,plane_end,out);

            plane_end = plane_iter;
            plane_iter -= width;

            for(int i = 0; i < width;++i)
                out[i] = pixel_average<typename IteratorType::value_type>()(out[i],plane_iter[i]);

            out -= width;
            std::copy(plane_iter,plane_end,out);

        }
        while(plane_iter != to);

        if(to == from)
            break;
        plane_iter = to;
    }
    while(1);
    return result;
}

template<class IteratorType,class OutputIterator>
OutputIterator upsampling_y_nearest(IteratorType from,IteratorType to,OutputIterator out,int width,int height)
{
    int plane_size = width*height;
    IteratorType plane_iter = to;
    IteratorType plane_end;
    out += (to-from) << 1;
    OutputIterator result = out;
    do
    {
        to -= plane_size;

        plane_end = plane_iter;
        plane_iter -= width;

        out -= width;
        std::copy(plane_iter,plane_end,out);
        out -= width;
        std::copy(plane_iter,plane_end,out);

        if(plane_iter != to)
        do{
            out -= width;
            std::copy(plane_iter,plane_end,out);
            plane_end = plane_iter;
            plane_iter -= width;
            std::copy(plane_iter,plane_end,out);
            out -= width;
            std::copy(plane_iter,plane_end,out);

        }
        while(plane_iter != to);

        if(to == from)
            break;
        plane_iter = to;
    }
    while(1);
    return result;
}

template<class IteratorType,class OutputIterator>
IteratorType upsampling_z(IteratorType from,IteratorType to,OutputIterator out,int width,int height,int depth)
{
    return upsampling_y(from,to,out,width*height,depth);
}

template<class IteratorType,class OutputIterator>
IteratorType upsampling_z_nearest(IteratorType from,IteratorType to,OutputIterator out,int width,int height,int depth)
{
    return upsampling_y_nearest(from,to,out,width*height,depth);
}

template<class ImageType1,class ImageType2>
void upsampling(const ImageType1& in,ImageType2& out)
{
    geometry<ImageType1::dimension> geo(in.geometry());
    geometry<ImageType1::dimension> new_geo(in.geometry());
    for(int dim = 0;dim < ImageType1::dimension;++dim)
        new_geo[dim] <<= 1;
    out.resize(new_geo);
    typename ImageType2::iterator end_iter =
            upsampling_x(in.begin(),in.begin()+geo.size(),out.begin(),geo.width());
    
    unsigned int plane_size = new_geo[0];
    for(int dim = 1;dim < ImageType1::dimension;++dim)
    {
        end_iter = upsampling_y(out.begin(),end_iter,out.begin(),plane_size,geo[dim]);
        plane_size *= new_geo[dim];
    }

}

template<class ImageType1,class ImageType2>
void upsampling_nearest(const ImageType1& in,ImageType2& out)
{
    geometry<ImageType1::dimension> geo(in.geometry());
    geometry<ImageType1::dimension> new_geo(in.geometry());
    for(int dim = 0;dim < ImageType1::dimension;++dim)
        new_geo[dim] <<= 1;
    out.resize(new_geo);
    typename ImageType2::iterator end_iter =
            upsampling_x_nearest(in.begin(),in.begin()+geo.size(),out.begin(),geo.width());

    unsigned int plane_size = new_geo[0];
    for(int dim = 1;dim < ImageType1::dimension;++dim)
    {
        end_iter = upsampling_y_nearest(out.begin(),end_iter,out.begin(),plane_size,geo[dim]);
        plane_size *= new_geo[dim];
    }

}

template<class ImageType>
void upsampling(ImageType& in)
{
    upsampling(in,in);
}


template<class ImageType>
void upsampling_nearest(ImageType& in)
{
    upsampling_nearest(in,in);
}

template<class value_type>
struct downsampling_facade{
    value_type operator()(value_type v1,value_type v2)
    {
        return (v1+v2)/((value_type)2);
    }
};
template<>
struct downsampling_facade<int>{
    int operator()(int v1,int v2)
    {
        v1 += v2;
        return v1 >= 0 ? v1>> 1: v1/2;
    }
};
template<>
struct downsampling_facade<short>{
    short operator()(short v1,short v2)
    {
        int v = v1;
        v += v2;
        return v >= 0 ? (v >> 1): v/2;
    }
};
template<>
struct downsampling_facade<char>{
    char operator()(char v1,char v2)
    {
        short v = v1;
        v += v2;
        return v >= 0 ? (v >> 1): v/2;
    }
};
template<>
struct downsampling_facade<unsigned int>{
    unsigned int operator()(unsigned int v1,unsigned int v2)
    {
         return (v1+v2) >> 1;
    }
};
template<>
struct downsampling_facade<unsigned short>{
    unsigned short operator()(unsigned short v1,unsigned short v2)
    {
         return ((unsigned int)v1+(unsigned int)v2) >> 1;
    }
};
template<>
struct downsampling_facade<unsigned char>{
    unsigned char operator()(unsigned char v1,unsigned char v2)
    {
         return ((unsigned short)v1+(unsigned short)v2) >> 1;
    }
};



template<class IteratorType,class OutputIterator>
OutputIterator downsampling_x(IteratorType from,IteratorType to,OutputIterator out,int width)
{
    typedef typename std::iterator_traits<IteratorType>::value_type value_type;
    downsampling_facade<value_type> average;
    int half_width = width >> 1;
    for(;from != to;from += width)
    {
        IteratorType read_end = from + (half_width << 1);
        for(IteratorType read = from;read != read_end;++out,read += 2)
            *out = average((*read),*(read+1));
    }
    return out;
}

template<class IteratorType,class OutputIterator>
OutputIterator downsampling_y(IteratorType from,IteratorType to,OutputIterator out,int width,int height)
{
    typedef typename std::iterator_traits<IteratorType>::value_type value_type;
    downsampling_facade<value_type> average;
    int half_height = height >> 1;
    int width2 = width << 1;
    int plane_size = width*height;
    int plane_size2 = half_height*width2;
    for(;from != to;from += plane_size)
    {
        IteratorType line_end = from+plane_size2;
        for(IteratorType line = from;line != line_end;line += width)
        {
            for(IteratorType end_line = line + width;line != end_line;++line,++out)
                *out = average(*line,line[width]);
        }
    }
    return out;
}
template<class IteratorType,class OutputIterator>
IteratorType downsampling_z(IteratorType from,IteratorType to,OutputIterator out,int width,int height,int depth)
{
    return downsampling_y(from,to,out,width*height,depth);
}

template<class ImageType1,class ImageType2>
void downsampling(const ImageType1& in,ImageType2& out)
{
    out.resize(in.geometry());
    geometry<ImageType1::dimension> new_geo(in.geometry());
    typename ImageType2::iterator end_iter = downsampling_x(in.begin(),in.end(),out.begin(),in.width());
    new_geo[0] >>= 1;
    unsigned int plane_size = new_geo[0];
    for(int dim = 1;dim < ImageType1::dimension;++dim)
    {
        end_iter = downsampling_y(out.begin(),end_iter,out.begin(),plane_size,in.geometry()[dim]);
        new_geo[dim] = (in.geometry()[dim] >> 1);
        plane_size *= new_geo[dim];
    }
    out.resize(new_geo);
}



template<class ImageType>
void downsampling(ImageType& in)
{
    downsampling(in,in);
}


template<typename image_type1,typename image_type2>
void downsample_with_padding(const image_type1& I,image_type2& rI)
{
    geometry<image_type1::dimension> pad_geo(I.geometry());
    for(unsigned int dim = 0;dim < image_type1::dimension;++dim)
        ++pad_geo[dim];
    basic_image<typename image_type1::value_type,image_type1::dimension> pad_I(pad_geo);
    image::draw(I,pad_I,pixel_index<image_type1::dimension>(I.geometry()));
    image::downsampling(pad_I,rI);
}

template<typename image_type1,typename image_type2,class geo_type>
void upsample_with_padding(const image_type1& I,image_type2& uI,const geo_type& geo)
{
    basic_image<typename image_type1::value_type,image_type1::dimension> new_I;
    image::upsampling(I,new_I);
    uI.resize(geo);
    image::draw(new_I,uI,pixel_index<image_type1::dimension>(I.geometry()));
}


template<class PixelType>
void shrink(const image::basic_image<PixelType,3>& image,
            image::basic_image<PixelType,3>& buffer,
            unsigned int scale)
{
    unsigned int slice_size = image.width()*image.height();
    const PixelType* slice = image.begin();
    buffer.resize(image::geometry<3>(
                      image.width()/scale,
                      image.height()/scale,
                      image.depth()/scale));
    std::fill(buffer.begin(),buffer.end(),0);
    for (unsigned int index = 0;index < buffer.depth();++index)
    {
        PixelType* buffer_iter = buffer.begin() + index*buffer.width()*buffer.height();
        image::basic_image<PixelType,2> buffer_slice(
            image::geometry<2>(image.width() /scale,
                               image.height() /scale));
        for (unsigned int j = 0;j < scale;++j,slice += slice_size)
        {
            std::fill(buffer_slice.begin(),buffer_slice.end(),0);

            const PixelType* line = slice;
            for (unsigned int k = 0;k < buffer_slice.height();++k)
            {
                PixelType* buffer_slice_iter = buffer_slice.begin() +
                                               k*buffer_slice.width();

                for (unsigned int l = 0;l < scale;++l,line += image.width())
                {

                    const PixelType* iter = line;
                    for (unsigned int m = 0;m < buffer_slice.width();++m)
                    {
                        PixelType sum = 0;
                        for (unsigned int n = 0;n < scale;++n,++iter)
                            sum += *iter > 0 ? *iter : 0;
                        buffer_slice_iter[m] += sum / scale;
                    }
                }
            }
            for (unsigned int k = 0;k < buffer_slice.size();++k)
                buffer_iter[k] += buffer_slice[k] / scale;
        }
    }
    for (unsigned int index = 0;index < buffer.size();++index)
        buffer[index] /= scale;
}

template<class PixelType>
void fast_resample(const image::basic_image<PixelType,3>& source_image,
                   image::basic_image<PixelType,3>& des_image)
{
    double dx = (double)(source_image.width()-1)/(double)(des_image.width()-1);
    double dy = (double)(source_image.height()-1)/(double)(des_image.height()-1);
    double dz = (double)(source_image.depth()-1)/(double)(des_image.depth()-1);
    double maxx = source_image.width()-1;
    double maxy = source_image.height()-1;
    double maxz = source_image.depth()-1;
    double coord[3];
    coord[2] = 0.5;
    interpolation<linear_weighting,3> interpolation;
    unsigned int wh = source_image.width()*source_image.height();
    unsigned int w = source_image.width();
    for (unsigned int z = 0,index = 0;z < des_image.depth();++z,coord[2] += dz)
    {
        if (coord[2] > maxz)
            coord[2] = maxz;
        unsigned int index_z = ((int)coord[2])*wh;
        coord[1] = 0.5;
        for (unsigned int y = 0;y < des_image.height();++y,coord[1] += dy)
        {
            if (coord[1] > maxy)
                coord[1] = maxy;
            unsigned int index_y = index_z + ((int)coord[1])*w;
            coord[0] = 0.5;
            for (unsigned int x = 0;x < des_image.width();++x,++index,coord[0] += dx)
            {
                if (coord[0] > maxx)
                    coord[0] = maxx;
                unsigned int index_x = index_y + ((int)coord[0]);
                des_image[index] = source_image[index_x];
            }
        }
    }
}

template<class PixelType>
void scale(const image::basic_image<PixelType,3>& source_image,
              image::basic_image<PixelType,3>& des_image)
{
    double dx = (double)(source_image.width()-1)/(double)(des_image.width()-1);
    double dy = (double)(source_image.height()-1)/(double)(des_image.height()-1);
    double dz = (double)(source_image.depth()-1)/(double)(des_image.depth()-1);
    double maxx = source_image.width()-1;
    double maxy = source_image.height()-1;
    double maxz = source_image.depth()-1;
    double coord[3];
    coord[2] = 0.0;
    for (unsigned int z = 0,index = 0;z < des_image.depth();++z,coord[2] += dz)
    {
        if (coord[2] > maxz)
            coord[2] = maxz;
        coord[1] = 0.0;
        for (unsigned int y = 0;y < des_image.height();++y,coord[1] += dy)
        {
            if (coord[1] > maxy)
                coord[1] = maxy;
            coord[0] = 0.0;
            for (unsigned int x = 0;x < des_image.width();++x,++index,coord[0] += dx)
            {
                if (coord[0] > maxx)
                    coord[0] = maxx;
                image::estimate(source_image,coord,des_image[index],linear);
            }
        }
    }
}

template<class PixelType>
void scale(const image::basic_image<PixelType,2>& source_image,
              image::basic_image<PixelType,2>& des_image)
{
    double dx = (double)(source_image.width()-1)/(double)(des_image.width()-1);
    double dy = (double)(source_image.height()-1)/(double)(des_image.height()-1);
    double maxx = source_image.width()-1;
    double maxy = source_image.height()-1;
    double coord[2];
    coord[1] = 0.0;
    for (unsigned int y = 0,index = 0;y < des_image.height();++y,coord[1] += dy)
    {
        if (coord[1] > maxy)
            coord[1] = maxy;
        coord[0] = 0.0;
        for (unsigned int x = 0;x < des_image.width();++x,++index,coord[0] += dx)
        {
            if (coord[0] > maxx)
                coord[0] = maxx;
            image::estimate(source_image,coord,des_image[index],linear);
        }
    }
}

template<class PixelType>
void scale_nearest(const image::basic_image<PixelType,2>& source_image,
              image::basic_image<PixelType,2>& des_image)
{
    double dx = (double)(source_image.width()-1)/(double)(des_image.width()-1);
    double dy = (double)(source_image.height()-1)/(double)(des_image.height()-1);
    double maxx = source_image.width()-1;
    double maxy = source_image.height()-1;
    double coord[2];
    coord[1] = 0.0;
    for (unsigned int y = 0,index = 0;y < des_image.height();++y,coord[1] += dy)
    {
        if (coord[1] > maxy)
            coord[1] = maxy;
        coord[0] = 0.0;
        for (unsigned int x = 0;x < des_image.width();++x,++index,coord[0] += dx)
        {
            if (coord[0] > maxx)
                coord[0] = maxx;
            int ix = std::floor(coord[0]+0.5);
            int iy = std::floor(coord[1]+0.5);
            if(source_image.geometry().is_valid(ix,iy))
                des_image[index] = source_image.at(ix,iy);
        }
    }
}


template<typename pixel_type>
void homogenize(image::basic_image<pixel_type,3>& I,image::basic_image<pixel_type,3>& J,int block_size = 20)
{
    if(I.geometry() != J.geometry())
        return;
    double r = image::correlation(I.begin(),I.end(),J.begin());
    if(r < 0.80)
    {
        image::normalize(I,*std::max_element(J.begin(),J.end()));
        return;
    }
    float distance_scale = 1.0/(float)block_size/(float)block_size;
    image::basic_image<float,3> v_map(I.geometry()),w_map(I.geometry());
    for(int z = block_size;z < J.depth()-block_size;z += block_size)
        for(int y = block_size;y < J.height()-block_size;y += block_size)
            for(int x = block_size;x < J.width()-block_size;x += block_size)
            {
                std::vector<image::pixel_index<3> > neighbors;
                image::get_neighbors(image::pixel_index<3>(x,y,z,I.geometry()),I.geometry(),block_size,neighbors);
                std::vector<float> Iv(neighbors.size()),Jv(neighbors.size()),dis2(neighbors.size());
                for(int i = 0; i < neighbors.size();++i)
                {
                    int dx = neighbors[i][0]-x;
                    int dy = neighbors[i][1]-y;
                    int dz = neighbors[i][2]-z;
                    dis2[i] = (dx*dx+dy*dy+dz*dz)*distance_scale;
                    Iv[i] = I[neighbors[i].index()];
                    Jv[i] = J[neighbors[i].index()];
                }
                double a,b,r2;
                image::linear_regression(Iv.begin(),Iv.end(),Jv.begin(),a,b,r2);
                for(int i = 0; i < neighbors.size();++i)
                {
                    float v = Iv[i]*a+b;
                    float w = std::exp(-dis2[i]*0.5)*r2;
                    if(w == 0.0)
                        continue;
                    int index = neighbors[i].index();
                    v_map[index] = (v_map[index]*w_map[index] + v*w)/(w_map[index]+w);
                    w_map[index] += w;
                }
            }
    image::upper_lower_threshold(v_map,0.0f,*std::max_element(J.begin(),J.end()));
    I = v_map;
}

template<class T>
void match_signal(const T& VG,T& VFF)
{
    std::vector<float> x,y;
    x.reserve(VG.size());
    y.reserve(VG.size());
    for(unsigned int index = 0;index < VG.size();++index)
        if(VG[index] > 0 && VFF[index] > 0)
        {
            x.push_back(VFF[index]);
            y.push_back(VG[index]);
        }
    std::pair<double,double> r = image::linear_regression(x.begin(),x.end(),y.begin());
    for(unsigned int index = 0;index < VG.size();++index)
        if(VG[index] > 0 && VFF[index] > 0)
            VFF[index] = std::max<float>(0,VFF[index]*r.first+r.second);
        else
            VFF[index] = 0;
}

template<class T>
void match_signal_kernel(const T& VG,T& VFF)
{
    typedef typename T::value_type value_type;
    value_type max_value = *std::max_element(VFF.begin(),VFF.end());

    std::vector<unsigned int> count(256);
    std::vector<float> sum(256);

    for(unsigned int index = 0;index < VG.size();++index)
        if(VG[index] > 0 && VFF[index] > 0)
        {
            int v = std::min<int>(255,std::round((float)VFF[index]/(float)max_value*255.499f));
            sum[v] += VG[index];
            ++count[v];
        }

    for(unsigned int index = 0;index < sum.size();++index)
    {
        if(count[index] != 0)
        {
            sum[index] /= count[index];
            continue;
        }
        if(index != 0)
            sum[index] = sum[index-1];
    }
    // smoothing
    image::basic_image<float,1> value(image::geometry<1>(256));
    value[0] = sum[0];
    value[255] = sum[255];
    for(unsigned int index = 1;index+1 < sum.size();++index)
        value[index] = (sum[index]+sum[index]+sum[index-1]+sum[index+1])*0.25;


    image::par_for(VG.size(),[&](int index)
    {
        if(VG[index] > 0 && VFF[index] > 0)
        {
            float v = (float)VFF[index]/(float)max_value*255.499f;
            float fv = std::floor(v);
            int floor_index = std::max<int>(0,fv);
            int ceil_index = std::min<int>(255,floor_index+1);
            float ceil_w = v-fv;
            float floor_w = 1.0f-ceil_w;
            VFF[index] = value[floor_index]*floor_w+value[ceil_index]*ceil_w;
        }
    else
        VFF[index] = 0;
    });
}

template<class PixelType,class CoordinateType,class ScaleVecType>
void resample(const image::basic_image<PixelType,3>& source_image,
              image::basic_image<PixelType,3>& des_image,
              const CoordinateType& from_position,
              const ScaleVecType& scales,
              interpolation_type type)
{
    CoordinateType z_base = from_position;
    for (unsigned int z = 0,index = 0;z < des_image.depth();++z)
    {
        CoordinateType y_base = z_base;
        for (unsigned int y = 0;y < des_image.height();++y)
        {
            CoordinateType position = y_base;
            for (unsigned int x = 0;x < des_image.width();++x,++index)
            {
                estimate(source_image,position,des_image[index],type);
                position += scales[0];
            }
            y_base += scales[1];
        }
        z_base += scales[2];
    }
}

template<class ImageType1,class ImageType2,class transform_type>
void resample_mt(const ImageType1& from,ImageType2& to,const transform_type& transform,interpolation_type type)
{
    to.for_each_mt([&transform,&from,type](typename ImageType2::value_type& value,
                                        image::pixel_index<ImageType1::dimension> index)
    {
        image::vector<ImageType1::dimension,double> pos;
        transform(index,pos);
        estimate(from,pos,value,type);
    });
}

template<class ImageType1,class ImageType2,class transform_type>
void resample(const ImageType1& from,ImageType2& to,const transform_type& transform,interpolation_type type)
{
    image::geometry<ImageType1::dimension> geo(to.geometry());
    for (image::pixel_index<ImageType1::dimension> index(geo);index < geo.size();++index)
    {
        image::vector<ImageType1::dimension,double> pos;
        transform(index,pos);
        estimate(from,pos,to[index.index()],type);
    }
}

template<class ImageType,class transform_type>
void resample(ImageType& from,const transform_type& transform,interpolation_type type)
{
    image::basic_image<class ImageType::value_type,ImageType::dimension> I(from.geometry());
    for (image::pixel_index<ImageType::dimension> index(from.geometry());index < from.size();++index)
    {
        image::vector<ImageType::dimension,double> pos;
        transform(index,pos);
        estimate(from,pos,I[index.index()],type);
    }
}


template<class ImageType1,class ImageType2,class value_type>
void resample(const ImageType1& from,ImageType2& to,const std::vector<value_type>& trans,interpolation_type type)
{
    image::transformation_matrix<float> transform;
    transform.load_from_transform(trans.begin());
    resample(from,to,transform,type);
}
template<class ImageType1,class ImageType2,int r,int c,class value_type>
void resample(const ImageType1& from,ImageType2& to,const image::matrix<r,c,value_type>& trans,interpolation_type type)
{
    image::transformation_matrix<float> transform;
    transform.load_from_transform(trans.begin());
    resample(from,to,transform,type);
}

template<class ImageType,class value_type>
void resample(ImageType& from,const std::vector<value_type>& trans,interpolation_type type)
{
    image::transformation_matrix<float> transform;
    transform.load_from_transform(trans.begin());
    resample(from,transform,type);
}
template<class ImageType,int r,int c,class value_type>
void resample(ImageType& from,const image::matrix<r,c,value_type>& trans,interpolation_type type)
{
    image::transformation_matrix<float> transform;
    transform.load_from_transform(trans.begin());
    resample(from,transform,type);
}

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 08/08/2017   Xiaojun
template<class ImageType1,class ImageType2>
void resample_linear(const ImageType1& from,ImageType2& to,interpolation_type type = interpolation_type::linear)
{
    image::vector<ImageType1::dimension> r;
    for(int i =0;i < ImageType1::dimension;++i)
        r[i] = ((float)from.geometry()[i]-1.0f)/((float)to.geometry()[i]-1.0f);
    for (image::pixel_index<ImageType1::dimension> index(to.geometry());index < to.size();++index)
    {
        image::vector<ImageType1::dimension> pos(index);
        image::multiply(pos,r);
        estimate(from,pos,to[index.index()],type);
    }
}
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 08/08/2017   Xiaojun
template<class ImageType1,class ImageType2>
void resample_cubic(const ImageType1& from,ImageType2& to,interpolation_type type = interpolation_type::cubic)
{
    image::vector<ImageType1::dimension> r;
    for(int i =0;i < ImageType1::dimension;++i)
        r[i] = ((float)from.geometry()[i]-1.0f)/((float)to.geometry()[i]-1.0f);
    for (image::pixel_index<ImageType1::dimension> index(to.geometry());index < to.size();++index)
    {
        image::vector<ImageType1::dimension> pos(index);
        image::multiply(pos,r);
        estimate(from,pos,to[index.index()],type);
    }
}
}
#endif
