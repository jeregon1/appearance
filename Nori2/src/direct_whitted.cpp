#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN
class DirectWhittedIntegrator: public Integrator {
public:
DirectWhittedIntegrator(const PropertyList& props) {
// No parameters this time
}
Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
Color3f Lo(0.);
// Find the surface that is visible in the requested direction
Intersection its;
if (!scene->rayIntersect(ray, its))
return scene->getBackground(ray);
float pdflight;
EmitterQueryRecord emitterRecord(its.p);
// Get all lights in the scene
const std::vector<Emitter*> lights = scene->getLights();
// Let's iterate over all emitters
for (unsigned int i = 0; i <lights.size(); ++i)
{
const Emitter* em = lights[i];
// Here we sample the point sources, getting its radiance
// and direction.
Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0.);
// Here perform a visibility query, to check whether the light
// source "em" is visible from the intersection point.
// For that, we create a ray object (shadow ray),
// and compute the intersection

// --------------------------------
// YOUR CODE HERE

// 1. Crear el rayo de sombra
// El origen es el punto de impacto (its.p)
// La dirección es hacia la luz (emitterRecord.wi)
Ray3f shadow_ray(its.p, emitterRecord.wi);

// 2. Comprobar la intersección
Intersection shadow_its;
if (scene->rayIntersect(shadow_ray, shadow_its)) {
    // El rayo ha chocado con algo.
    // Ahora comprobamos si el choque está ANTES de llegar a la luz.
    // Si la distancia al objeto es menor que la distancia a la luz, hay sombra.
    if (shadow_its.t < emitterRecord.dist) {
        continue; // Ignoramos esta luz y pasamos a la siguiente (sombra)
    }
}
// --------------------------------

// Finally, we evaluate the BSDF. For that, we need to build
// a BSDFQueryRecord from the outgoing direction (the direction
// of the primary ray, in ray.d), and the incoming direction
// (the direction to the light source, in emitterRecord.wi).
// Note that: a) the BSDF assumes directions in the local frame
// of reference; and b) that both the incoming and outgoing
// directions are assumed to start from the intersection point.
BSDFQueryRecord bsdfRecord(its.toLocal(-ray.d),
its.toLocal(emitterRecord.wi), its.uv, ESolidAngle);
// For each light, we accomulate the incident light times the
// foreshortening times the BSDF term (i.e. the render equation).
Lo += Le *its.shFrame.n.dot(emitterRecord.wi) *its.mesh->getBSDF()->eval(bsdfRecord);
}
return Lo;
}
std::string toString() const {
return "Direct Whitted Integrator []";
}
};
NORI_REGISTER_CLASS(DirectWhittedIntegrator, "direct_whitted");
NORI_NAMESPACE_END