// By Emil Ernerfeldt 2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
#include "imgui_sdl_soft.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "imgui.h"


namespace imgui_sw
{


ColorBuffer color_buffer_;


namespace
{

struct Texture
{
	const ImU8* const pixels_; // 8-bit.
	const int width_;
	const int height_;


	Texture(
		const ImU8* const pixels,
		const int width,
		const int height)
		:
		pixels_(pixels),
		width_(width),
		height_(height)
	{
	}
}; // Texture

struct PaintTarget
{
	ImU32* pixels_;
	int width_;
	int height_;
	ImVec2 scale_; // Multiply ImGui (point) coordinates with this to get pixel coordinates.
}; // PaintTarget

// ----------------------------------------------------------------------------

struct ColorInt
{
	ImU32 a_;
	ImU32 b_;
	ImU32 g_;
	ImU32 r_;


	ColorInt()
		:
		a_(),
		b_(),
		g_(),
		r_()
	{
	}

	ColorInt(
		const ImU32 a,
		const ImU32 b,
		const ImU32 g,
		const ImU32 r)
		:
		a_(a),
		b_(b),
		g_(g),
		r_(r)
	{
	}

	explicit ColorInt(
		const ImU32 x)
		:
		a_((x >> IM_COL32_A_SHIFT) & 0xFFU),
		b_((x >> IM_COL32_B_SHIFT) & 0xFFU),
		g_((x >> IM_COL32_G_SHIFT) & 0xFFU),
		r_((x >> IM_COL32_R_SHIFT) & 0xFFU)
	{
	}

	ImU32 toUint32() const
	{
		return (a_ << 24U) | (b_ << 16U) | (g_ << 8U) | r_;
	}
}; // ColorInt


ColorInt blend_0_x(
	const ColorInt& source)
{
	return ColorInt(
		0, // Whatever.
		(source.b_ * source.a_) / 255,
		(source.g_ * source.a_) / 255,
		(source.r_ * source.a_) / 255
	);
}

ColorInt blend(
	const ColorInt& target,
	const ColorInt& source)
{
	return ColorInt(
		0, // Whatever.
		((source.b_ * source.a_) + (target.b_ * (255 - source.a_))) / 255,
		((source.g_ * source.a_) + (target.g_ * (255 - source.a_))) / 255,
		((source.r_ * source.a_) + (target.r_ * (255 - source.a_))) / 255
	);
}

// ----------------------------------------------------------------------------
// Used for interpolating vertex attributes (color and texture coordinates) in a triangle.

struct Barycentric
{
	float w0_;
	float w1_;
	float w2_;


	Barycentric()
	{
	}

	Barycentric(
		const float w0,
		const float w1,
		const float w2)
		:
		w0_(w0),
		w1_(w1),
		w2_(w2)
	{
	}
}; // Barycentric


Barycentric operator*(
	const float f,
	const Barycentric& va)
{
	return Barycentric(f * va.w0_, f * va.w1_, f * va.w2_);
}

void operator+=(
	Barycentric& a,
	const Barycentric& b)
{
	a.w0_ += b.w0_;
	a.w1_ += b.w1_;
	a.w2_ += b.w2_;
}

Barycentric operator+(
	const Barycentric& a,
	const Barycentric& b)
{
	return Barycentric(a.w0_ + b.w0_, a.w1_ + b.w1_, a.w2_ + b.w2_);
}

// ----------------------------------------------------------------------------
// Useful operators on ImGui vectors:

ImVec2 operator*(
	const float f,
	const ImVec2& v)
{
	return ImVec2(f * v.x, f * v.y);
}

ImVec2 operator+(
	const ImVec2& a,
	const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(
	const ImVec2& a,
	const ImVec2& b)
{
	return ImVec2(a.x - b.x, a.y - b.y);
}

bool operator!=(
	const ImVec2& a,
	const ImVec2& b)
{
	return a.x != b.x || a.y != b.y;
}

ImVec4 operator*(
	const float f,
	const ImVec4& v)
{
	return ImVec4(f * v.x, f * v.y, f * v.z, f * v.w);
}

ImVec4 operator+(
	const ImVec4& a,
	const ImVec4& b)
{
	return ImVec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

// ----------------------------------------------------------------------------
// Copies of functions in ImGui, inlined for speed:

ImVec4 color_convert_u32_to_float4(
	const ImU32 in)
{
	const float s = 1.0F / 255.0F;

	return ImVec4(
		((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
		((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
		((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
		((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

ImU32 color_convert_float4_to_u32(
	const ImVec4& in)
{
	return
		(static_cast<ImU32>((in.x * 255.0F) + 0.5F) << IM_COL32_R_SHIFT) |
		(static_cast<ImU32>((in.y * 255.0F) + 0.5F) << IM_COL32_G_SHIFT) |
		(static_cast<ImU32>((in.z * 255.0F) + 0.5F) << IM_COL32_B_SHIFT) |
		(static_cast<ImU32>((in.w * 255.0F) + 0.5F) << IM_COL32_A_SHIFT)
	;
}

// ----------------------------------------------------------------------------
// For fast and subpixel-perfect triangle rendering we used fixed point arithmetic.
// To keep the code simple we use 64 bits to avoid overflows.

typedef ImS64 Int;

static const Int fixed_bias = 256;


struct Point
{
	Int x_;
	Int y_;


	Point(
		const Int x,
		const Int y)
		:
		x_(x),
		y_(y)
	{
	}
};

Int orient2d(
	const Point& a,
	const Point& b,
	const Point& c)
{
	return ((b.x_ - a.x_) * (c.y_ - a.y_)) - ((b.y_ - a.y_) * (c.x_ - a.x_));
}

Int as_int(
	const float v)
{
	return static_cast<Int>(std::floor(v * fixed_bias));
}

Point as_point(
	const ImVec2& v)
{
	return Point(as_int(v.x), as_int(v.y));
}

// ----------------------------------------------------------------------------

float min3(
	const float a,
	const float b,
	const float c)
{
	if (a < b && a < c)
	{
		return a;
	}

	return b < c ? b : c;
}

float max3(
	const float a,
	const float b,
	const float c)
{
	if (a > b && a > c)
	{
		return a;
	}

	return b > c ? b : c;
}

float barycentric(
	const ImVec2& a,
	const ImVec2& b,
	const ImVec2& point)
{
	return ((b.x - a.x) * (point.y - a.y)) - ((b.y - a.y) * (point.x - a.x));
}

ImU8 sample_texture(
	const Texture& texture,
	const ImVec2& uv)
{
	int tx = static_cast<int>((uv.x * (texture.width_ - 1.0F)) + 0.5F);
	int ty = static_cast<int>((uv.y * (texture.height_ - 1.0F)) + 0.5F);

	// Clamp to inside of texture:
	tx = std::max(tx, 0);
	tx = std::min(tx, texture.width_ - 1);
	ty = std::max(ty, 0);
	ty = std::min(ty, texture.height_ - 1);

	return texture.pixels_[(ty * texture.width_) + tx];
}

void paint_uniform_rectangle(
	const PaintTarget& target,
	const ImVec2& min_f,
	const ImVec2& max_f,
	const ColorInt& color)
{
	// Integer bounding box [min, max):
	int min_x_i = static_cast<int>((target.scale_.x * min_f.x) + 0.5F);
	int min_y_i = static_cast<int>((target.scale_.y * min_f.y) + 0.5F);
	int max_x_i = static_cast<int>((target.scale_.x * max_f.x) + 0.5F);
	int max_y_i = static_cast<int>((target.scale_.y * max_f.y) + 0.5F);

	// Clamp to render target:
	min_x_i = std::max(min_x_i, 0);
	min_y_i = std::max(min_y_i, 0);
	max_x_i = std::min(max_x_i, target.width_);
	max_y_i = std::min(max_y_i, target.height_);

	// We often blend the same colors over and over again, so optimize for this (saves 25% total cpu):
	ImU32 last_target_pixel = target.pixels_[(min_y_i * target.width_) + min_x_i];
	ImU32 last_output = blend(ColorInt(last_target_pixel), color).toUint32();

	for (int y = min_y_i; y < max_y_i; ++y)
	{
		for (int x = min_x_i; x < max_x_i; ++x)
		{
			ImU32& target_pixel = target.pixels_[(y * target.width_) + x];

			if (target_pixel != last_target_pixel)
			{
				last_target_pixel = target_pixel;
				target_pixel = blend(ColorInt(target_pixel), color).toUint32();
				last_output = target_pixel;
			}

			target_pixel = last_output;
		}
	}
}

void paint_uniform_textured_rectangle(
	const PaintTarget& target,
	const Texture& texture,
	const ImVec4& clip_rect,
	const ImDrawVert& min_v,
	const ImDrawVert& max_v)
{
	const ImVec2 min_p = ImVec2(target.scale_.x * min_v.pos.x, target.scale_.y * min_v.pos.y);
	const ImVec2 max_p = ImVec2(target.scale_.x * max_v.pos.x, target.scale_.y * max_v.pos.y);

	// Find bounding box:
	float min_x_f = min_p.x;
	float min_y_f = min_p.y;
	float max_x_f = max_p.x;
	float max_y_f = max_p.y;

	// Clip against clip_rect:
	min_x_f = std::max(min_x_f, target.scale_.x * clip_rect.x);
	min_y_f = std::max(min_y_f, target.scale_.y * clip_rect.y);
	max_x_f = std::min(max_x_f, (target.scale_.x * clip_rect.z) - 0.5F);
	max_y_f = std::min(max_y_f, (target.scale_.y * clip_rect.w) - 0.5F);

	// Integer bounding box [min, max):
	int min_x_i = static_cast<int>(min_x_f);
	int min_y_i = static_cast<int>(min_y_f);
	int max_x_i = static_cast<int>(max_x_f + 1.0F);
	int max_y_i = static_cast<int>(max_y_f + 1.0F);

	// Clip against render target:
	min_x_i = std::max(min_x_i, 0);
	min_y_i = std::max(min_y_i, 0);
	max_x_i = std::min(max_x_i, target.width_);
	max_y_i = std::min(max_y_i, target.height_);

	const ImVec2 topleft = ImVec2(
		min_x_i + (0.5F * target.scale_.x),
		min_y_i + (0.5F * target.scale_.y));

	const ImVec2 delta_uv_per_pixel
	(
		(max_v.uv.x - min_v.uv.x) / (max_p.x - min_p.x),
		(max_v.uv.y - min_v.uv.y) / (max_p.y - min_p.y)
	);

	const ImVec2 uv_topleft
	(
		min_v.uv.x + ((topleft.x - min_v.pos.x) * delta_uv_per_pixel.x),
		min_v.uv.y + ((topleft.y - min_v.pos.y) * delta_uv_per_pixel.y)
	);

	ImVec2 current_uv = uv_topleft;

	for (int y = min_y_i; y < max_y_i; ++y, current_uv.y += delta_uv_per_pixel.y)
	{
		current_uv.x = uv_topleft.x;

		for (int x = min_x_i; x < max_x_i; ++x, current_uv.x += delta_uv_per_pixel.x)
		{
			ImU32& target_pixel = target.pixels_[(y * target.width_) + x];

			const ImU8 texel = sample_texture(texture, current_uv);

			// The font texture is all black or all white, so optimize for this:
			if (texel == 0)
			{
				continue;
			}

			if (texel == 255)
			{
				target_pixel = min_v.col;

				continue;
			}

			// Other textured rectangles
			ColorInt source_color(min_v.col);
			source_color.a_ = (source_color.a_ * texel) / 255;
			target_pixel = blend(ColorInt(target_pixel), source_color).toUint32();
		}
	}
}

// When two triangles share an edge, we want to draw the pixels on that edge exactly once.
// The edge will be the same, but the direction will be the opposite
// (assuming the two triangles have the same winding order).
// Which edge wins? This functions decides.
bool is_dominant_edge(
	const ImVec2& edge)
{
	// return edge.x < 0 || (edge.x == 0 && edge.y > 0);
	return edge.y > 0 || (edge.y == 0 && edge.x < 0);
}

// Handles triangles in any winding order (CW/CCW)
void paint_triangle(
	const PaintTarget& target,
	const Texture* texture,
	const ImVec4& clip_rect,
	const ImDrawVert& v0,
	const ImDrawVert& v1,
	const ImDrawVert& v2)
{
	const ImVec2 p0 = ImVec2(target.scale_.x * v0.pos.x, target.scale_.y * v0.pos.y);
	const ImVec2 p1 = ImVec2(target.scale_.x * v1.pos.x, target.scale_.y * v1.pos.y);
	const ImVec2 p2 = ImVec2(target.scale_.x * v2.pos.x, target.scale_.y * v2.pos.y);

	// Can be positive or negative depending on winding order
	const float rect_area = barycentric(p0, p1, p2);

	if (rect_area == 0)
	{
		return;
	}

	// if (rect_area < 0.0f) { return paint_triangle(target, texture, clip_rect, v0, v2, v1, stats); }


	// ------------------------------------------------------------------------
	const bool has_uniform_color = (v0.col == v1.col && v0.col == v2.col);
	const bool use_bary = (!has_uniform_color || texture);

	const ImVec4 c0 = color_convert_u32_to_float4(v0.col);
	const ImVec4 c1 = (has_uniform_color ? ImVec4() : color_convert_u32_to_float4(v1.col));
	const ImVec4 c2 = (has_uniform_color ? ImVec4() : color_convert_u32_to_float4(v2.col));


	// ------------------------------------------------------------------------
	// Find bounding box:
	float min_x_f = min3(p0.x, p1.x, p2.x);
	float min_y_f = min3(p0.y, p1.y, p2.y);
	float max_x_f = max3(p0.x, p1.x, p2.x);
	float max_y_f = max3(p0.y, p1.y, p2.y);

	// Clip against clip_rect:
	min_x_f = std::max(min_x_f, target.scale_.x * clip_rect.x);
	min_y_f = std::max(min_y_f, target.scale_.y * clip_rect.y);
	max_x_f = std::min(max_x_f, (target.scale_.x * clip_rect.z) - 0.5F);
	max_y_f = std::min(max_y_f, (target.scale_.y * clip_rect.w) - 0.5F);

	// Integer bounding box [min, max):
	int min_x_i = static_cast<int>(min_x_f);
	int min_y_i = static_cast<int>(min_y_f);
	int max_x_i = static_cast<int>(max_x_f + 1);
	int max_y_i = static_cast<int>(max_y_f + 1);

	// Clip against render target:
	min_x_i = std::max(min_x_i, 0);
	min_y_i = std::max(min_y_i, 0);
	max_x_i = std::min(max_x_i, target.width_);
	max_y_i = std::min(max_y_i, target.height_);

	// ------------------------------------------------------------------------
	// Set up interpolation of barycentric coordinates:

	Barycentric bary_topleft;
	Barycentric bary_dx;
	Barycentric bary_dy;
	Barycentric bary_current_row;

	if (use_bary)
	{
		const ImVec2 topleft = ImVec2(
			min_x_i + (0.5F * target.scale_.x),
			min_y_i + (0.5F * target.scale_.y));

		static const ImVec2 dx = ImVec2(1, 0);
		static const ImVec2 dy = ImVec2(0, 1);

		const float w0_topleft = barycentric(p1, p2, topleft);
		const float w1_topleft = barycentric(p2, p0, topleft);
		const float w2_topleft = barycentric(p0, p1, topleft);

		const float w0_dx = barycentric(p1, p2, topleft + dx) - w0_topleft;
		const float w1_dx = barycentric(p2, p0, topleft + dx) - w1_topleft;
		const float w2_dx = barycentric(p0, p1, topleft + dx) - w2_topleft;

		const float w0_dy = barycentric(p1, p2, topleft + dy) - w0_topleft;
		const float w1_dy = barycentric(p2, p0, topleft + dy) - w1_topleft;
		const float w2_dy = barycentric(p0, p1, topleft + dy) - w2_topleft;

		static const Barycentric bary_0(1, 0, 0);
		static const Barycentric bary_1(0, 1, 0);
		static const Barycentric bary_2(0, 0, 1);

		const float inv_area = 1 / rect_area;

		bary_topleft = inv_area * ((w0_topleft * bary_0) + (w1_topleft * bary_1) + (w2_topleft * bary_2));
		bary_dx = inv_area * ((w0_dx * bary_0) + (w1_dx * bary_1) + (w2_dx * bary_2));
		bary_dy = inv_area * ((w0_dy * bary_0) + (w1_dy * bary_1) + (w2_dy * bary_2));

		bary_current_row = bary_topleft;
	}

	// ------------------------------------------------------------------------
	// For pixel-perfect inside/outside testing:

	const int sign = rect_area > 0 ? 1 : -1; // winding order?

	const int bias0i = is_dominant_edge(p2 - p1) ? 0 : -1;
	const int bias1i = is_dominant_edge(p0 - p2) ? 0 : -1;
	const int bias2i = is_dominant_edge(p1 - p0) ? 0 : -1;

	const Point p0i = as_point(p0);
	const Point p1i = as_point(p1);
	const Point p2i = as_point(p2);

	// ------------------------------------------------------------------------

	// We often blend the same colors over and over again, so optimize for this (saves 10% total cpu):
	const ColorInt v0_col_int(v0.col);

	ImU32 last_target_pixel = 0;
	ImU32 last_output = blend_0_x(v0_col_int).toUint32();

	Point p((fixed_bias * min_x_i) + fixed_bias / 2, (fixed_bias * min_y_i) + fixed_bias / 2);

	ImU32* target_pixels = &target.pixels_[min_y_i * target.width_];

	for (int y = min_y_i; y < max_y_i; ++y)
	{
		Barycentric bary;

		if (use_bary)
		{
			bary = bary_current_row;
		}

		bool has_been_inside_this_row = false;

		Int w0i = (sign * orient2d(p1i, p2i, p)) + bias0i;
		const Int d_w0i = fixed_bias * sign * (p1i.y_ - p2i.y_);

		Int w1i = (sign * orient2d(p2i, p0i, p)) + bias1i;
		const Int d_w1i = fixed_bias * sign * (p2i.y_ - p0i.y_);

		Int w2i = (sign * orient2d(p0i, p1i, p)) + bias2i;
		const Int d_w2i = fixed_bias * sign * (p0i.y_ - p1i.y_);

		for (int x = min_x_i; x < max_x_i; ++x)
		{
			if (use_bary)
			{
				bary += bary_dx;
			}

			if (w0i < 0 || w1i < 0 || w2i < 0)
			{
				if (has_been_inside_this_row)
				{
					// Gives a nice 10% speedup

					break;
				}
			}
			else
			{
				has_been_inside_this_row = true;

				ImU32& target_pixel = target_pixels[x];

				if (has_uniform_color && !texture)
				{
					if (target_pixel != last_target_pixel)
					{
						last_target_pixel = target_pixel;
						target_pixel = blend(ColorInt(target_pixel), v0_col_int).toUint32();
						last_output = target_pixel;
					}

					target_pixel = last_output;
				}
				else
				{
					const float w0 = bary.w0_;
					const float w1 = bary.w1_;
					const float w2 = bary.w2_;

					ImVec4 src_color;

					if (has_uniform_color)
					{
						src_color = c0;
					}
					else
					{
						src_color = (w0 * c0) + (w1 * c1) + (w2 * c2);
					}

					if (texture)
					{
						const ImVec2 uv = (w0 * v0.uv) + (w1 * v1.uv) + (w2 * v2.uv);
						src_color.w *= sample_texture(*texture, uv) / 255.0F;
					}

					if (src_color.w >= 1)
					{
						// Opaque, no blending needed:

						target_pixel = color_convert_float4_to_u32(src_color);
					}
					else if (src_color.w > 0)
					{
						const ImVec4 target_color = color_convert_u32_to_float4(target_pixel);
						const ImVec4 blended_color = (src_color.w * src_color) + (1.0F - src_color.w) * target_color;
						target_pixel = color_convert_float4_to_u32(blended_color);
					}
				}
			}

			w0i += d_w0i;
			w1i += d_w1i;
			w2i += d_w2i;
		}

		target_pixels += target.width_;

		p.y_ += fixed_bias;

		if (use_bary)
		{
			bary_current_row += bary_dy;
		}
	}
}

void paint_draw_cmd(
	const PaintTarget& target,
	const ImDrawVert* vertices,
	const ImDrawIdx* idx_buffer,
	const ImDrawCmd& pcmd)
{
	const Texture* texture = reinterpret_cast<const Texture*>(pcmd.TextureId);

	assert(texture != NULL);

	// ImGui uses the first pixel for "white".
	const ImVec2 white_uv = ImVec2(0.5F / texture->width_, 0.5F / texture->height_);

	for (unsigned i = 0; i + 3 <= pcmd.ElemCount; )
	{
		const ImDrawVert& v0 = vertices[idx_buffer[i + 0]];
		const ImDrawVert& v1 = vertices[idx_buffer[i + 1]];
		const ImDrawVert& v2 = vertices[idx_buffer[i + 2]];

		// Text is common, and is made of textured rectangles. So let's optimize for it.
		// This assumes the ImGui way to layout text does not change.
		if (i + 6 <= pcmd.ElemCount &&
			idx_buffer[i + 3] == idx_buffer[i + 0] &&
			idx_buffer[i + 4] == idx_buffer[i + 2])
		{
			const ImDrawVert& v3 = vertices[idx_buffer[i + 5]];

			if (v0.pos.x == v3.pos.x &&
				v1.pos.x == v2.pos.x &&
				v0.pos.y == v1.pos.y &&
				v2.pos.y == v3.pos.y &&
				v0.uv.x == v3.uv.x &&
				v1.uv.x == v2.uv.x &&
				v0.uv.y == v1.uv.y &&
				v2.uv.y == v3.uv.y)
			{
				const bool has_uniform_color =
					v0.col == v1.col &&
					v0.col == v2.col &&
					v0.col == v3.col;

				const bool has_texture =
					v0.uv != white_uv ||
					v1.uv != white_uv ||
					v2.uv != white_uv ||
					v3.uv != white_uv;

				if (has_uniform_color && has_texture)
				{
					paint_uniform_textured_rectangle(target, *texture, pcmd.ClipRect, v0, v2);
					i += 6;

					continue;
				}
			}
		}

		// A lot of the big stuff are uniformly colored rectangles,
		// so we can save a lot of CPU by detecting them:
		if (i + 6 <= pcmd.ElemCount)
		{
			const ImDrawVert& v3 = vertices[idx_buffer[i + 3]];
			const ImDrawVert& v4 = vertices[idx_buffer[i + 4]];
			const ImDrawVert& v5 = vertices[idx_buffer[i + 5]];

			ImVec2 min;
			ImVec2 max;

			min.x = min3(v0.pos.x, v1.pos.x, v2.pos.x);
			min.y = min3(v0.pos.y, v1.pos.y, v2.pos.y);
			max.x = max3(v0.pos.x, v1.pos.x, v2.pos.x);
			max.y = max3(v0.pos.y, v1.pos.y, v2.pos.y);

			// Not the prettiest way to do this, but it catches all cases
			// of a rectangle split into two triangles.
			// TODO: Stop it from also assuming duplicate triangles is one rectangle.
			if ((v0.pos.x == min.x || v0.pos.x == max.x) &&
				(v0.pos.y == min.y || v0.pos.y == max.y) &&
				(v1.pos.x == min.x || v1.pos.x == max.x) &&
				(v1.pos.y == min.y || v1.pos.y == max.y) &&
				(v2.pos.x == min.x || v2.pos.x == max.x) &&
				(v2.pos.y == min.y || v2.pos.y == max.y) &&
				(v3.pos.x == min.x || v3.pos.x == max.x) &&
				(v3.pos.y == min.y || v3.pos.y == max.y) &&
				(v4.pos.x == min.x || v4.pos.x == max.x) &&
				(v4.pos.y == min.y || v4.pos.y == max.y) &&
				(v5.pos.x == min.x || v5.pos.x == max.x) &&
				(v5.pos.y == min.y || v5.pos.y == max.y))
			{
				const bool has_uniform_color =
					v0.col == v1.col &&
					v0.col == v2.col &&
					v0.col == v3.col &&
					v0.col == v4.col &&
					v0.col == v5.col;

				const bool has_texture =
					v0.uv != white_uv ||
					v1.uv != white_uv ||
					v2.uv != white_uv ||
					v3.uv != white_uv ||
					v4.uv != white_uv ||
					v5.uv != white_uv;

				min.x = std::max(min.x, pcmd.ClipRect.x);
				min.y = std::max(min.y, pcmd.ClipRect.y);
				max.x = std::min(max.x, pcmd.ClipRect.z - 0.5F);
				max.y = std::min(max.y, pcmd.ClipRect.w - 0.5F);

				if (max.x < min.x || max.y < min.y)
				{
					// Completely clipped
					i += 6;

					continue;
				}

				if (has_uniform_color && !has_texture)
				{
					paint_uniform_rectangle(target, min, max, ColorInt(v0.col));
					i += 6;

					continue;
				}
			}
		}

		const bool has_texture = (v0.uv != white_uv || v1.uv != white_uv || v2.uv != white_uv);

		paint_triangle(target, has_texture ? texture : NULL, pcmd.ClipRect, v0, v1, v2);

		i += 3;
	}
}

void paint_draw_list(
	const PaintTarget& target,
	const ImDrawList* cmd_list)
{
	const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer[0];
	const ImDrawVert* vertices = cmd_list->VtxBuffer.Data;

	const int cmd_buffer_size = cmd_list->CmdBuffer.size();

	for (int cmd_i = 0; cmd_i < cmd_buffer_size; ++cmd_i)
	{
		const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_i];

		if (pcmd.UserCallback)
		{
			pcmd.UserCallback(cmd_list, &pcmd);
		}
		else
		{
			paint_draw_cmd(target, vertices, idx_buffer, pcmd);
		}

		idx_buffer += pcmd.ElemCount;
	}
}


} // namespace


void bind_imgui_painting()
{
	ImGuiIO& io = ImGui::GetIO();

	// Load default font (embedded in code):
	ImU8* tex_data;

	int font_width;
	int font_height;
	io.Fonts->GetTexDataAsAlpha8(&tex_data, &font_width, &font_height);

	Texture* texture = new Texture(tex_data, font_width, font_height);

	io.Fonts->TexID = texture;
}

void paint_imgui(
	ImU32* const pixels,
	const int width_pixels,
	const int height_pixels)
{
	const float width_points = ImGui::GetIO().DisplaySize.x;
	const float height_points = ImGui::GetIO().DisplaySize.y;

	const ImVec2 scale(width_pixels / width_points, height_pixels / height_points);

	PaintTarget target = {pixels, width_pixels, height_pixels, scale};

	const ImDrawData* draw_data = ImGui::GetDrawData();

	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		paint_draw_list(target, draw_data->CmdLists[i]);
	}
}

void unbind_imgui_painting()
{
	ImGuiIO& io = ImGui::GetIO();
	delete reinterpret_cast<Texture*>(io.Fonts->TexID);
	io.Fonts = NULL;
}


} // imgui_sw
