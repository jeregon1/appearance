/*
	This file is an extension of Nori, a simple educational ray tracer
	Copyright (c) 2020 by Adrian Jarabo

	v2 - Nov 2024
	Nestor: Added HDR textures and scale_x, scale_y usage

	Nori is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License Version 3
	as published by the Free Software Foundation.

	Nori is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/texture.h>
#include <nori/bitmap.h>

#include <filesystem/resolver.h>
#include <fstream>


NORI_NAMESPACE_BEGIN
class BitmapTexture: public Texture {
public:
	BitmapTexture(const PropertyList& props) {
		m_bitmap = 0;

		m_bitmap_name = props.getString("filename", "null");
		filesystem::path filename =
			getFileResolver()->resolve(m_bitmap_name);

		std::ifstream is(filename.str());
		if (!is.fail())
		{
			cout << "Loading Texture Map: " << filename.str() << endl;

			m_bitmap = new LDRBitmap(filename.str());
			cout << "Loaded " << m_bitmap_name << " - SIZE [" << m_bitmap->rows() << ", " << m_bitmap->cols() << "]" << endl;
		}
		m_color = props.getColor("color", Color3f(1.));
		m_scale[0] = props.getFloat("scalex", 1.f);
		m_scale[1] = props.getFloat("scaley", 1.f);
	}
	~BitmapTexture()
	{
		if (m_bitmap)
			delete m_bitmap;

		m_bitmap = 0;
	}

	virtual std::string toString() const {
		return tfm::format(
			"Texture[\n"
			"  scale = %s,\n"
			"  name = %s,\n"
			"]",
			m_color.toString(),
			m_bitmap_name);
	}

	virtual Color3f eval(const Point2f& uv) const {

		if (!m_bitmap)
			return m_color;
		
		// This allows to flip/scale/stretch textures using m_scale (scaling is centered at 0.5, 0.5)
		// Point2f warped_uv = .5f * ((uv*2.f) - 1.f) * m_scale + .5f; // Eigen complains about this
		float u = .5f * ((uv[0]*2.f) - 1.f) * m_scale[0] + .5f;
		float v = .5f * ((uv[1]*2.f) - 1.f) * m_scale[1] + .5f;

		return m_bitmap->eval(Point2f(u,v)) * m_color;
	}

protected:
	Color3f m_color;
	LDRBitmap* m_bitmap;
	float m_rotation;
	Vector2f m_scale;

	std::string m_bitmap_name;
};

class HDRBitmapTexture: public Texture {
public:
	HDRBitmapTexture(const PropertyList& props) {
		m_bitmap = 0;

		m_bitmap_name = props.getString("filename", "null");
		filesystem::path filename =
			getFileResolver()->resolve(m_bitmap_name);

		std::ifstream is(filename.str());
		if (!is.fail())
		{
			cout << "Loading Texture Map: " << filename.str() << endl;

			m_bitmap = new Bitmap(filename.str());
			cout << "Loaded " << m_bitmap_name << " - SIZE [" << m_bitmap->rows() << ", " << m_bitmap->cols() << "]" << endl;
		}
		m_color = props.getColor("color", Color3f(1.));
		m_scale[0] = props.getFloat("scalex", 1.f);
		m_scale[1] = props.getFloat("scaley", 1.f);
		m_brightness = props.getFloat("brightness", 1.f);

	}
	~HDRBitmapTexture()
	{
		if (m_bitmap)
			delete m_bitmap;

		m_bitmap = 0;
	}

	virtual std::string toString() const {
		return tfm::format(
			"Texture[\n"
			"  scale = %s,\n"
			"  name = %s,\n"
			"]",
			m_color.toString(),
			m_bitmap_name);
	}

	virtual Color3f eval(const Point2f& uv) const {

		if (!m_bitmap)
			return m_color;
		
		
		// This allows to flip/scale/stretch textures using m_scale (scaling is centered at 0.5, 0.5)
		// Point2f warped_uv = .5f * ((uv*2.f) - 1.f) * m_scale + .5f; // Eigen complains about this
		float u = .5f * ((uv[0]*2.f) - 1.f) * m_scale[0] + .5f;
		float v = .5f * ((uv[1]*2.f) - 1.f) * m_scale[1] + .5f;

		return m_bitmap->eval(Point2f(u,v)) * m_color * m_brightness;
	}

protected:
	Color3f m_color;
	Bitmap* m_bitmap;
	float m_rotation, m_brightness;
	Vector2f m_scale;

	std::string m_bitmap_name;
};


class CheckerTexture : public Texture {
public:
	CheckerTexture(const PropertyList& props) {
		m_color1 = props.getColor("color", Color3f(.9f));
		m_color2 = props.getColor("color", Color3f(.1f));
		m_scale[0] = props.getFloat("scalex", 1.f);
		m_scale[1] = props.getFloat("scaley", 1.f);
	}
	~CheckerTexture()
	{
	}

	virtual std::string toString() const {
		return tfm::format(
			"CheckerTexture[\n"
			"  color1 = %s, color2 = %s\n"
			"  scale = %s,\n"
			"]",
			m_color1.toString(),m_color2.toString(),
			m_scale.toString()
			);
	}

	virtual Color3f eval(const Point2f& uv) const {
		Point2f pos = uv.cwiseProduct(m_scale);
		int px = int(pos[0]); // clamp to integers
		int py = int(pos[1]);

		if (mod(px, 2) == mod(py, 2))
			return m_color1;
		else
			return m_color2;
	}

protected:
	Color3f m_color1,m_color2;
	Point2f m_scale;
};


NORI_REGISTER_CLASS(BitmapTexture, "textmap")
NORI_REGISTER_CLASS(HDRBitmapTexture, "hdrtextmap")
NORI_REGISTER_CLASS(CheckerTexture, "checker")
NORI_NAMESPACE_END
