#include <nori/integrator.h>
#include <nori/scene.h>

#include <nori/bsdf.h>
 
NORI_NAMESPACE_BEGIN

class AOVIntegrator : public Integrator {
public:
    AOVIntegrator(const PropertyList &props) {
        m_mode = props.getString("variable", "albedo");
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        // Find the surface that is visible in the requested direction 
        Intersection its;
        if (!scene->rayIntersect(ray, its)) 
            return scene->getBackground(ray); // no intersection

        if (m_mode == "normals") {
            Normal3f n = its.shFrame.n; // get the normal from the intersection
            // Map the normal from [-1,1] to [0,1] for visualization
            return .5f + .5f * Color3f(n.x(), n.y(), n.z());
        } else if (m_mode == "depth") {
            throw NoriException("AOVIntegrator: You need to implement the depth mode!");
        } 
        else if (m_mode == "uv") {
            return Color3f(its.uv.x(), its.uv.y(), 0.f);
        }
        else if (m_mode == "albedo") {
            const BSDF *bsdf = its.mesh->getBSDF(); // get the material
            // Get the base color (albedo) at this point of the surface:
            Color3f albedo = bsdf->getAlbedo(its.uv);
            return albedo;
        }
        else {
            throw NoriException("AOVIntegrator: Unimplemented mode \"%s\"!", m_mode.c_str());
        }
    }

    std::string toString() const {
        return "AOVIntegrator[mode=" + m_mode + "]"; 
    }
protected:
    std::string m_mode;
};

NORI_REGISTER_CLASS(AOVIntegrator, "aov");
NORI_NAMESPACE_END
