#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/emittersampling.h>

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

        // Muestrear una luz y sumar su contribucion directa (Shadow Ray incluido)
        Lo += sampleEmitterDirect(scene, sampler, its, its.toLocal(-ray.d), false);

        return Lo;
    }

    std::string toString() const override {
        return "Direct Emitter Sampling Integrator []";
    }
};

NORI_REGISTER_CLASS(DirectEmitterSampling, "direct_ems");
NORI_NAMESPACE_END
