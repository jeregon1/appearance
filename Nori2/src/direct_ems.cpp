#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class DirectEmitterSampling : public Integrator {
public:
    DirectEmitterSampling(const PropertyList& props) {
        // No parameters needed
    }

    Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const override {
        Color3f Lo(0.0f);

        // Find the surface that is visible in the requested direction
        Intersection its;
        if (!scene->rayIntersect(ray, its)) {
            return scene->getBackground(ray);
        }

        // Añade la radiancia de los emisores directamente visibles (Le term)
        if (its.mesh->isEmitter()) {
            EmitterQueryRecord evalRecord(its.p);
            evalRecord.wi = ray.d;
            evalRecord.n = its.shFrame.n;
            evalRecord.uv = its.uv;
            Lo += its.mesh->getEmitter()->eval(evalRecord);
        }

        // Muestrear aleatoriamente UNA sola luz de la escena
        float pdflight;
        const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);
        
        // Si no hay luces o la probabilidad es nula, devolvemos lo que tengamos
        if (!em || pdflight <= 0.0f) {
            return Lo;
        }

        // Muestrear un punto en la luz seleccionada
        EmitterQueryRecord emitterRecord(its.p);
        Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0.0f);

        // Comprobacion de visibilidad (Shadoe Ray)
        Ray3f shadow_ray(its.p, emitterRecord.wi);
        Intersection shadow_its;
        bool is_occluded = false;

        if (scene->rayIntersect(shadow_ray, shadow_its)) {
            // Le restamos un pequeño epsilon (1e-4f) a la distancia para evitar el auto_sombreado
            if (shadow_its.t < emitterRecord.dist - 1e-4f) {
                is_occluded = true;
            }
        }

        // Si la luz es visible, evaluamos la ecuacion de renderizado
        if (!is_occluded) {
            BSDFQueryRecord bsdfRecord(its.toLocal(-ray.d), its.toLocal(emitterRecord.wi), its.uv, ESolidAngle);
            float cosTheta = its.shFrame.n.dot(emitterRecord.wi);

            // Solo se suma si la luz viene por encima de la superficie
            if (cosTheta > 0.0f) {
                // Monte Carlo: Multiplicamos por la BSDF y el Coseno, y DIVIDIMOS por la PDF de elegir la luz.
                // (La radiancia Le devuelta por em->sample() ya viene dividida por la PDF espacial del area de la luz).
                Lo += (Le * cosTheta * its.mesh->getBSDF()->eval(bsdfRecord)) / pdflight;
            }
        }

        return Lo;
    }

    std::string toString() const override {
        return "Direct Emitter Sampling Integrator []";
    }
};

NORI_REGISTER_CLASS(DirectEmitterSampling, "direct_ems");
NORI_NAMESPACE_END
