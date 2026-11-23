#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

class DirectMaterialSampling : public Integrator {
public:
    DirectMaterialSampling(const PropertyList &props) {}

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const override {
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return scene->getBackground(ray);

        Color3f Lo(0.0f);

        // Emision propia (Le)
        if (its.mesh->isEmitter()) {
            EmitterQueryRecord evalRecord(its.p);
            evalRecord.wi = ray.d;
            evalRecord.n = its.shFrame.n;
            evalRecord.uv = its.uv;
            Lo += its.mesh->getEmitter()->eval(evalRecord);
        }

        // Muestrear dirección con la BRDF (devuelve fr * cosTheta / pdf)
        const BSDF *bsdf = its.mesh->getBSDF();
        BSDFQueryRecord bRec(its.toLocal(-ray.d));
        bRec.uv = its.uv;
        Color3f bsdfWeight = bsdf->sample(bRec, sampler->next2D());

        if (bsdfWeight.isZero())
            return Lo;

        Ray3f nextRay(its.p, its.toWorld(bRec.wo));
        Intersection nextIts;

        if (scene->rayIntersect(nextRay, nextIts)) {
            if (nextIts.mesh->isEmitter()) {
                EmitterQueryRecord evalRecord(its.p);
                evalRecord.wi = nextRay.d;
                evalRecord.n = nextIts.shFrame.n;
                evalRecord.uv = nextIts.uv;
                Lo += bsdfWeight * nextIts.mesh->getEmitter()->eval(evalRecord);
            }
        } else {
            // Golpea el Environment Map
            Lo += bsdfWeight * scene->getBackground(nextRay);
        }

        return Lo;
    }

    std::string toString() const override {
        return "DirectMaterialSampling[]";
    }
};

NORI_REGISTER_CLASS(DirectMaterialSampling, "direct_mats");
NORI_NAMESPACE_END