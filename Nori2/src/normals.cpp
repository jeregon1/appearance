#include <nori/integrator.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN

class NormalIntegrator : public Integrator {
    public:
        NormalIntegrator(const PropertyList &props) {

        }
    
    Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f &ray) const {
        // Encuentra la superficie visible en la direccion solicitada
        Intersection its;
        if (!scene->rayIntersect(ray, its))
        return Color3f(0.0f); // Si el rayo no choca con nada devuelve negro

        // Devuelve el valor absoluto componente a componente
        // de la normal de sombreado como un color
        Normal3f n = its.shFrame.n.cwiseAbs();
        return Color3f(n.x(), n.y(), n.z());
    }

    std::string toString() const {
        return "NormalIntegrator[]";
    }
};

NORI_REGISTER_CLASS(NormalIntegrator, "normals");
NORI_NAMESPACE_END