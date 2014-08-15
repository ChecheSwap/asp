#pragma once

#include <asp/Segmentation.hpp>
#include <slimage/image.hpp>

namespace asp
{
	namespace detail
	{
		template<typename T>
		struct SPlot
		{
			slimage::Image3ub vis;
			Segmentation<T> seg;

			SPlot(const Segmentation<T>& seg)
			:	vis{seg.indices.width(), seg.indices.height(), slimage::Pixel3ub{0,0,0}},
				seg(seg)
			{}

			operator const slimage::Image3ub&() const
			{ return vis; }
		};

		template<typename F>
		struct SDense
		{
			F colfnc;
			slimage::Pixel3ub invalid;
		};

		struct SBorder
		{
			slimage::Pixel3ub color;
		};

		template<typename T, typename F>
		SPlot<T> operator<<(SPlot<T>&& p, const SDense<F>& u)
		{
			for(size_t i=0; i<p.seg.indices.size(); i++) {
				int sid = p.seg.indices[i];
				p.vis[i] = (sid >= 0) ? u.colfnc(p.seg.superpixels[sid]) : u.invalid;
			}
			return p;
		}

		template<typename T>
		SPlot<T> operator<<(SPlot<T>&& p, const SBorder& u)
		{
			const int width = p.vis.width();
			const int height = p.vis.height();
			for(int y=0; y<height; y++) {
				int ym = std::max(y-1, 0);
				int yp = std::min(y+1, height);
				for(int x=0; x<width; x++) {
					int xm = std::max(x-1, 0);
					int xp = std::min(x+1, width);
					int i = p.seg.indices(x,y);
					if(    i != p.seg.indices(xm,y)
						|| i != p.seg.indices(xp,y)
						|| i != p.seg.indices(x,ym)
						|| i != p.seg.indices(x,yp)) {
						p.vis(x,y) = u.color;
					}
				}
			}
			return p;
		}
	}

	template<typename T>
	detail::SPlot<T> Plot(const Segmentation<T>& seg)
	{ return detail::SPlot<T>(seg); }

	template<typename F>
	detail::SDense<F> PlotDense(F colfnc)
	{ return {
		colfnc,
		slimage::Pixel3ub{255,0,255}
	}; }

	inline
	detail::SBorder PlotBorder(const slimage::Pixel3ub& color = slimage::Pixel3ub{0,0,0})
	{ return {
		color
	}; }

	template<typename T>
	slimage::Image3ub PlotColor(const Segmentation<T>& seg)
	{
		return Plot(seg)
			<< PlotDense(
				[](const Pixel<T>& u) {
					return slimage::Pixel3ub{
						static_cast<unsigned char>(255.0f*u.data.color[0]),
						static_cast<unsigned char>(255.0f*u.data.color[1]),
						static_cast<unsigned char>(255.0f*u.data.color[2])
					};
				})
			<< PlotBorder();
	}

}
