#pragma once

#include <asp/segmentation.hpp>
#include <asp/graph.hpp>
#include <slimage/image.hpp>
#include <slimage/algorithm.hpp>

namespace asp
{
	namespace detail
	{
		inline unsigned char uf32_to_ui08(float x)
		{ return static_cast<unsigned char>(255.0f*x); }

		inline unsigned char sf32_to_ui08(float x)
		{ return static_cast<unsigned char>(255.0f*0.5f*(x + 1.0f)); }

		inline slimage::Pixel3ub uf32_to_ui08(const Eigen::Vector3f& x)
		{ return slimage::Pixel3ub{uf32_to_ui08(x[0]), uf32_to_ui08(x[1]), uf32_to_ui08(x[2])}; }

		inline slimage::Pixel3ub sf32_to_ui08(const Eigen::Vector3f& x)
		{ return slimage::Pixel3ub{sf32_to_ui08(x[0]), sf32_to_ui08(x[1]), sf32_to_ui08(x[2])}; }

		template<typename T>
		struct PlotHelperInit
		{
			slimage::Image3ub vis;
			Segmentation<T> seg;

			PlotHelperInit(const Segmentation<T>& seg)
			:	vis{seg.indices.width(), seg.indices.height(), slimage::Pixel3ub{0,0,0}},
				seg(seg)
			{}

			operator const slimage::Image3ub&() const
			{ return vis; }
		};

		template<typename F>
		struct PlotHelperPixels
		{
			F colfnc;
		};

		template<typename T, typename F>
		PlotHelperInit<T> operator<<(PlotHelperInit<T>&& p, const PlotHelperPixels<F>& u)
		{
			for(size_t i=0; i<p.seg.input.size(); i++) {
				p.vis[i] = u.colfnc(p.seg.input[i]);
			}
			return p;
		}

		template<typename F>
		struct PlotHelperSuperpixels
		{
			F colfnc;
			slimage::Pixel3ub invalid;
		};

		template<typename T, typename F>
		PlotHelperInit<T> operator<<(PlotHelperInit<T>&& p, const PlotHelperSuperpixels<F>& u)
		{
			for(size_t i=0; i<p.seg.indices.size(); i++) {
				int sid = p.seg.indices[i];
				p.vis[i] = (sid >= 0) ? u.colfnc(p.seg.superpixels[sid]) : u.invalid;
			}
			return p;
		}

		struct PlotHelperBorder
		{
			slimage::Pixel3ub color;
		};

		template<typename T>
		PlotHelperInit<T> operator<<(PlotHelperInit<T>&& p, const PlotHelperBorder& u)
		{
			const int width = p.vis.width();
			const int height = p.vis.height();
			for(int y=0; y<height; y++) {
				int ym = std::max(y-1, 0);
				int yp = std::min(y+1, height-1);
				for(int x=0; x<width; x++) {
					int xm = std::max(x-1, 0);
					int xp = std::min(x+1, width-1);
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

		struct PlotHelperGraph
		{
			slimage::Pixel3ub color;
		};

		template<typename T>
		PlotHelperInit<T> operator<<(PlotHelperInit<T>&& p, const PlotHelperGraph& u)
		{
			// plot edges
			for(const auto& eid : detail::as_range(boost::edges(p.seg.graph))) {
				const auto& p1 = p.seg.superpixels[boost::source(eid, p.seg.graph)].position;
				const auto& p2 = p.seg.superpixels[boost::target(eid, p.seg.graph)].position;
				slimage::PaintLine(p.vis, p1[0], p1[1], p2[0], p2[1], u.color);
			}
			// plot superpixels
			for(const auto& vid : detail::as_range(boost::vertices(p.seg.graph))) {
				const auto& s = p.seg.superpixels[vid];
				float r = 0.5f * s.radius;
				slimage::FillBox(p.vis, s.position[0]-r, s.position[1]-r, 2.0f*r, 2.0f*r, uf32_to_ui08(s.data.color));
			}
			return p;
		}
		
	}

	template<typename T>
	detail::PlotHelperInit<T> Plot(const Segmentation<T>& seg)
	{ return detail::PlotHelperInit<T>(seg); }

	template<typename F>
	detail::PlotHelperPixels<F> PlotPixels(F colfnc)
	{ return {
		colfnc
	}; }

	template<typename F>
	detail::PlotHelperSuperpixels<F> PlotSuperpixels(F colfnc)
	{ return {
		colfnc,
		slimage::Pixel3ub{255,0,255}
	}; }

	inline
	detail::PlotHelperBorder PlotBorder(const slimage::Pixel3ub& color = slimage::Pixel3ub{0,0,0})
	{ return {
		color
	}; }

	inline
	detail::PlotHelperGraph PlotGraph(const slimage::Pixel3ub& color = slimage::Pixel3ub{255,255,255})
	{ return {
		color
	}; }

	template<typename T>
	slimage::Image3ub VisualizePixelDensity(const Segmentation<T>& seg)
	{
		return Plot(seg)
			<< PlotPixels([](const Pixel<T>& u) {
				constexpr float DMIN = 0.000f;
				constexpr float DMAX = 0.025f;
				float v = std::min(std::max(0.0f, (u.density-DMIN)/(DMAX-DMIN)), 1.0f);
				unsigned char c = static_cast<unsigned char>(255.0f*v);
				return slimage::Pixel3ub{c,c,c};
			});
	}

	template<typename T>
	slimage::Image3ub VisualizeSuperpixelColor(const Segmentation<T>& seg)
	{
		return Plot(seg)
			<< PlotSuperpixels([](const Pixel<T>& u) { return detail::uf32_to_ui08(u.data.color); })
			<< PlotBorder();
	}

	template<typename T>
	slimage::Image3ub VisualizeSuperpixelNormal(const Segmentation<T>& seg)
	{
		return Plot(seg)
			<< PlotSuperpixels([](const Pixel<T>& u) { return detail::sf32_to_ui08(u.data.normal); })
			<< PlotBorder();
	}

	template<typename T>
	slimage::Image3ub VisualizeSuperpixelGraph(const Segmentation<T>& seg)
	{
		return Plot(seg) << PlotGraph();
	}

}
