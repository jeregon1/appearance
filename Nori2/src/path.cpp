#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

// Path tracing ingenuo: solo muestreo de BSDF; la luz se recoge al chocar con un emisor.
class PathTracing : public Integrator {
public:
    PathTracing(const PropertyList &props) {}

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &cameraRay) const override {
        Color3f Lo(0.0f);
        Color3f throughput(1.0f);
        Ray3f ray = cameraRay;

        for (int depth = 0; ; ++depth) {
            Intersection its;
            if (!scene->rayIntersect(ray, its)) {
                Lo += throughput * scene->getBackground(ray);
                break;
            }

            if (its.mesh->isEmitter()) {
                const Emitter *em = its.mesh->getEmitter();
                EmitterQueryRecord eRec(em, ray.o, its.p, its.shFrame.n, its.uv);
                Lo += throughput * em->eval(eRec);
            }

            // Ruleta rusa: a partir del tercer rebote el camino sobrevive con probabilidad q,
            // y se divide entre q para que el estimador siga siendo insesgado
            if (depth >= 2) {
                float q = std::min(throughput.maxCoeff(), 0.95f);
                if (sampler->next1D() >= q)
                    break;
                throughput /= q;
            }

            BSDFQueryRecord bRec(its.toLocal(-ray.d), its.uv);
            Color3f bsdfWeight = its.mesh->getBSDF()->sample(bRec, sampler->next2D());
            if (bsdfWeight.isZero())
                break;

            // sample() ya devuelve f * cos / pdf
            throughput *= bsdfWeight;
            ray = Ray3f(its.p, its.toWorld(bRec.wo));
        }

        return Lo;
    }

    std::string toString() const override {
        return "PathTracing[]";
    }
};

NORI_REGISTER_CLASS(PathTracing, "path");
NORI_NAMESPACE_END
