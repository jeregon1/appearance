#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>
#include <nori/emittersampling.h>

NORI_NAMESPACE_BEGIN

// Path tracing con NEE y MIS: en cada rebote la luz directa se estima con las dos tecnicas
// (muestreo de luz y muestreo de BSDF), combinadas con la heuristica de balance.
class PathTracingMIS : public Integrator {
public:
    PathTracingMIS(const PropertyList &props) {}

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &cameraRay) const override {
        Color3f Lo(0.0f);
        Color3f throughput(1.0f);
        Ray3f ray = cameraRay;
        // Densidad (angulo solido) con la que la BSDF del rebote anterior genero 'ray'.
        // El rayo de camara y los rebotes especulares no tienen alternativa con muestreo
        // de luz, asi que su emision lleva peso 1.
        float bsdfPdf = 0.0f;
        bool fullWeight = true;

        for (int depth = 0; ; ++depth) {
            Intersection its;
            if (!scene->rayIntersect(ray, its)) {
                const Emitter *env = scene->getEnvironmentalEmitter();
                float envPdf = 0.0f;
                Color3f Le = scene->getBackground(ray, envPdf);
                if (!Le.isZero()) {
                    float w = fullWeight ? 1.0f : miWeight(bsdfPdf, scene->pdfEmitter(env) * envPdf);
                    Lo += w * throughput * Le;
                }
                break;
            }

            if (its.mesh->isEmitter()) {
                const Emitter *em = its.mesh->getEmitter();
                EmitterQueryRecord eRec(em, ray.o, its.p, its.shFrame.n, its.uv);
                Color3f Le = em->eval(eRec);
                if (!Le.isZero()) {
                    float w = fullWeight ? 1.0f : miWeight(bsdfPdf, scene->pdfEmitter(em) * em->pdf(eRec));
                    Lo += w * throughput * Le;
                }
            }

            if (depth >= 2) {
                float q = std::min(throughput.maxCoeff(), 0.95f);
                if (sampler->next1D() >= q)
                    break;
                throughput /= q;
            }

            const BSDF *bsdf = its.mesh->getBSDF();
            BSDFQueryRecord bRec(its.toLocal(-ray.d), its.uv);
            Color3f bsdfWeight = bsdf->sample(bRec, sampler->next2D());
            bool discrete = bRec.measure == EDiscrete;

            if (!discrete)
                Lo += throughput * sampleEmitterDirect(scene, sampler, its, its.toLocal(-ray.d), true);

            if (bsdfWeight.isZero())
                break;

            throughput *= bsdfWeight;
            fullWeight = discrete;
            bsdfPdf = discrete ? 0.0f : bsdf->pdf(bRec);
            ray = Ray3f(its.p, its.toWorld(bRec.wo));
        }

        return Lo;
    }

    std::string toString() const override {
        return "PathTracingMIS[]";
    }
};

NORI_REGISTER_CLASS(PathTracingMIS, "path_mis");
NORI_NAMESPACE_END
