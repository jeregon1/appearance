#pragma once

#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN

/// Balance heuristic weight of the strategy with density pdfA, combined with one of density pdfB
inline float miWeight(float pdfA, float pdfB) {
    float denom = pdfA + pdfB;
    return (denom > 0.0f) ? (pdfA / denom) : 0.0f;
}

/**
 * \brief Estimates the direct light reflected at 'its' toward 'wiLocal' by sampling one emitter
 *
 * Returns f * Le * cos / pdf for the sampled light point, or zero if it is occluded.
 * With 'mis' set, non-delta emitters are weighted by the balance heuristic against
 * the BSDF sampling density of the same direction.
 */
inline Color3f sampleEmitterDirect(const Scene *scene, Sampler *sampler, const Intersection &its,
                                   const Vector3f &wiLocal, bool mis) {
    float pdflight = 0.0f;
    const Emitter *em = scene->sampleEmitter(sampler->next1D(), pdflight);
    if (em == nullptr || pdflight <= 0.0f)
        return Color3f(0.0f);

    EmitterQueryRecord eRec(its.p);
    Color3f Le = em->sample(eRec, sampler->next2D(), 0.0f);
    if (Le.isZero())
        return Color3f(0.0f);

    Ray3f shadowRay(its.p, eRec.wi);
    Intersection shadowIts;
    if (scene->rayIntersect(shadowRay, shadowIts) && shadowIts.t < eRec.dist - 1e-4f)
        return Color3f(0.0f);

    const BSDF *bsdf = its.mesh->getBSDF();
    BSDFQueryRecord bRec(wiLocal, its.toLocal(eRec.wi), its.uv, ESolidAngle);
    float cosTheta = Frame::cosTheta(bRec.wo);
    if (cosTheta <= 0.0f)
        return Color3f(0.0f);

    Color3f f = bsdf->eval(bRec);
    if (f.isZero())
        return Color3f(0.0f);

    float weight = 1.0f;
    if (mis && !em->isDelta())
        weight = miWeight(pdflight * eRec.pdf, bsdf->pdf(bRec));

    return weight * Le * f * cosTheta / pdflight;
}

NORI_NAMESPACE_END
