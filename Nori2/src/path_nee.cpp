#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>
#include <nori/emittersampling.h>

NORI_NAMESPACE_BEGIN

// Path tracing con Next-Event Estimation: en cada rebote se muestrea una luz (luz directa)
// y se continua el camino muestreando la BSDF (luz indirecta).
class PathTracingNEE : public Integrator {
public:
    PathTracingNEE(const PropertyList &props) {}

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &cameraRay) const override {
        Color3f Lo(0.0f);
        Color3f throughput(1.0f);
        Ray3f ray = cameraRay;
        // La emision que encuentra un rayo de BSDF ya se conto con NEE en el rebote anterior.
        // Solo se suma para el rayo de camara o tras un rebote especular (EDiscrete),
        // porque esas direcciones no se pueden muestrear desde la luz.
        bool countEmission = true;

        for (int depth = 0; ; ++depth) {
            Intersection its;
            if (!scene->rayIntersect(ray, its)) {
                if (countEmission)
                    Lo += throughput * scene->getBackground(ray);
                break;
            }

            if (countEmission && its.mesh->isEmitter()) {
                const Emitter *em = its.mesh->getEmitter();
                EmitterQueryRecord eRec(em, ray.o, its.p, its.shFrame.n, its.uv);
                Lo += throughput * em->eval(eRec);
            }

            if (depth >= 2) {
                float q = std::min(throughput.maxCoeff(), 0.95f);
                if (sampler->next1D() >= q)
                    break;
                throughput /= q;
            }

            // Se muestrea la BSDF antes que la luz para saber si es perfectamente especular
            BSDFQueryRecord bRec(its.toLocal(-ray.d), its.uv);
            Color3f bsdfWeight = its.mesh->getBSDF()->sample(bRec, sampler->next2D());
            bool discrete = bRec.measure == EDiscrete;

            if (!discrete)
                Lo += throughput * sampleEmitterDirect(scene, sampler, its, its.toLocal(-ray.d), false);

            if (bsdfWeight.isZero())
                break;

            throughput *= bsdfWeight;
            countEmission = discrete;
            ray = Ray3f(its.p, its.toWorld(bRec.wo));
        }

        return Lo;
    }

    std::string toString() const override {
        return "PathTracingNEE[]";
    }
};

NORI_REGISTER_CLASS(PathTracingNEE, "path_nee");
NORI_NAMESPACE_END
