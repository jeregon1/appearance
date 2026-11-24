#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>
#include <nori/frame.h>
#include <nori/emittersampling.h>

NORI_NAMESPACE_BEGIN

class DirectMIS : public Integrator {
public:
    DirectMIS(const PropertyList &props) {}

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const override {
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return scene->getBackground(ray);

        Color3f Lo(0.0f);

        // Emision directa de la superficie si es luz (sin peso MIS)
        if (its.mesh->isEmitter()) {
            EmitterQueryRecord evalRecord(its.mesh->getEmitter(), ray.o, its.p, its.shFrame.n, its.uv);
            Lo += its.mesh->getEmitter()->eval(evalRecord);
        }

        const BSDF *bsdf = its.mesh->getBSDF();

        // Prueba 1: Muestreo de emisor, ponderado con MIS
        Lo += sampleEmitterDirect(scene, sampler, its, its.toLocal(-ray.d), true);

        // Prueba 2: Muestreo de material (BRDF)
        {
            BSDFQueryRecord bRec(its.toLocal(-ray.d));
            bRec.uv = its.uv;
            Color3f bsdfWeight = bsdf->sample(bRec, sampler->next2D());

            if (!bsdfWeight.isZero()) {
                Ray3f nextRay(its.p, its.toWorld(bRec.wo));
                Intersection nextIts;

                const Emitter *hitEm = nullptr;
                Color3f Le(0.0f);
                float pdf_em = 0.0f;

                if (scene->rayIntersect(nextRay, nextIts)) {
                    if (nextIts.mesh->isEmitter()) {
                        hitEm = nextIts.mesh->getEmitter();
                        EmitterQueryRecord eRec(hitEm, its.p, nextIts.p, nextIts.shFrame.n, nextIts.uv);
                        Le = hitEm->eval(eRec);
                        if (!Le.isZero()) {
                            pdf_em = scene->pdfEmitter(hitEm) * hitEm->pdf(eRec);
                        }
                    }
                } else {
                    // El rayo escapa al entorno
                    hitEm = scene->getEnvironmentalEmitter();
                    if (hitEm != nullptr) {
                        EmitterQueryRecord eRec(its.p);
                        eRec.wi = nextRay.d;
                        eRec.dist = std::numeric_limits<float>::infinity();
                        Le = hitEm->eval(eRec);
                        if (!Le.isZero()) {
                            pdf_em = scene->pdfEmitter(hitEm) * hitEm->pdf(eRec);
                        }
                    }
                }

                if (hitEm != nullptr && !Le.isZero()) {
                    float w_mat = 1.0f;
                    // En materiales delta Dirac no se aplica peso MIS
                    if (bRec.measure != EDiscrete) {
                        float pdf_mat = bsdf->pdf(bRec);
                        w_mat = miWeight(pdf_mat, pdf_em);
                    }
                    Lo += w_mat * bsdfWeight * Le;
                }
            }
        }

        return Lo;
    }

    std::string toString() const override {
        return "DirectMIS[]";
    }
};

NORI_REGISTER_CLASS(DirectMIS, "direct_mis");
NORI_NAMESPACE_END