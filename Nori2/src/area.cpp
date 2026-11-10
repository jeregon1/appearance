/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob
	
	v1 - Dec 01 2020
	Copyright (c) 2020 by Adrian Jarabo

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

#include <nori/emitter.h>
#include <nori/warp.h>
#include <nori/mesh.h>
#include <nori/texture.h>

NORI_NAMESPACE_BEGIN

class AreaEmitter : public Emitter {
public:
	AreaEmitter(const PropertyList &props) {
		m_type = EmitterType::EMITTER_AREA;
		m_radiance = new ConstantSpectrumTexture(props.getColor("radiance", Color3f(1.f)));
		m_scale = props.getFloat("scale", 1.);
	}

	virtual std::string toString() const {
		return tfm::format(
			"AreaLight[\n"
			"  radiance = %s,\n"
			"  scale = %f,\n"
			"]",
			m_radiance->toString(), m_scale);
	}

	// We don't assume anything about the visibility of points specified in 'ref' and 'p' in the EmitterQueryRecord.
	virtual Color3f eval(const EmitterQueryRecord & lRec) const {
        if (!m_mesh)
            throw NoriException("There is no shape attached to this Area light!");

        // This function call can be done by bsdf sampling routines.
        // Hence the ray was already traced for us - i.e a visibility test was already performed.
        // Hence just check if the associated normal in emitter query record and incoming direction are not backfacing
        
        // La luz viaja desde la superficie de la luz (lRec.n) hacia el punto sombreado.
        // lRec.wi es la dirección DESDE el punto sombreado HACIA la luz.
        // Comprobamos si incide en la cara frontal del emisor (dot product normal luz y -wi).
        float cosTheta = lRec.n.dot(-lRec.wi);

        if (cosTheta <= 0.0f) {
            return Color3f(0.0f); // Estamos viendo la parte de atrás de la luz (no emite)
        }

        // 2. Evaluamos la textura de radiancia en el punto UV
        return m_scale * m_radiance->eval(lRec.uv);
    }

	virtual Color3f sample(EmitterQueryRecord & lRec, const Point2f & sample, float optional_u) const {
        if (!m_mesh)
            throw NoriException("There is no shape attached to this Area light!");

        // Muestrear una posicion en la malla (rellena p, n y uv)
        m_mesh->samplePosition(sample, lRec.p, lRec.n, lRec.uv);

        // Calcular la direccion desde el punto a sombrear hacia la luz y su distancia
        Vector3f w = lRec.p - lRec.ref;
        lRec.dist = w.norm();
        lRec.wi = w / lRec.dist;

        // Comprobar si la luz está orientada de espaldas al punto
        float cosTheta = lRec.n.dot(-lRec.wi);
        if (cosTheta <= 0.0f) {
            lRec.pdf = 0.0f;
            return Color3f(0.0f);
        }

        // Calcular la PDF (convirtiendo de area a angulo solido) y la radiancia devuelta
        lRec.pdf = pdf(lRec);

        return (m_scale * m_radiance->eval(lRec.uv)) / lRec.pdf;
    }

	// Returns probability with respect to solid angle given by all the information inside the emitterqueryrecord.
	// Assumes all information about the intersection point is already provided inside.
	// WARNING: Use with care. Malformed EmitterQueryRecords can result in undefined behavior. 
	//			Plus no visibility is considered.
	virtual float pdf(const EmitterQueryRecord &lRec) const {
        if (!m_mesh)
            throw NoriException("There is no shape attached to this Area light!");

        float cosTheta = lRec.n.dot(-lRec.wi);
        if (cosTheta <= 0.0f) {
            return 0.0f;
        }

        // Ecuacion 10: p_omega = p_A * (dist^2) / cosTheta
        float pdf_area = m_mesh->pdf(lRec.p);
        return pdf_area * (lRec.dist * lRec.dist) / cosTheta;
    }


	virtual Color3f getRadiance() const {
		return m_scale * m_radiance->eval(Point2f(0.5f, 0.5f));
	}

	// Get the parent mesh
	void setParent(NoriObject *parent)
	{
		auto type = parent->getClassType();
		if (type == EMesh)
			m_mesh = static_cast<Mesh*>(parent);
	}

	// Set children
	void addChild(NoriObject* obj, const std::string& name = "none") {
		switch (obj->getClassType()) {
		case ETexture:
			if (name == "radiance")
			{
				delete m_radiance;
				m_radiance = static_cast<Texture*>(obj);
			}
			else
				throw NoriException("AreaEmitter::addChild(<%s>,%s) is not supported!",
					classTypeName(obj->getClassType()), name);
			break;

		default:
			throw NoriException("AreaEmitter::addChild(<%s>) is not supported!",
				classTypeName(obj->getClassType()));
		}
	}
protected:
	Texture* m_radiance;
	float m_scale;
};

NORI_REGISTER_CLASS(AreaEmitter, "area")
NORI_NAMESPACE_END
