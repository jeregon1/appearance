#include <nori/integrator.h>
#include <nori/scene.h>
 
NORI_NAMESPACE_BEGIN

class NDotLIntegrator : public Integrator {
public:
    NDotLIntegrator(const PropertyList &props) {
        // Read the properties from the xml, or set some defaults:
        m_light_dir = props.getVector("light_direction", Vector3f(0.f,-1.f,0.f));
        m_light_dir = m_light_dir.normalized(); // direction vectors should be normalized
        m_light_color = props.getColor("light_color", Color3f(1.f)); 
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        // Find the surface that is visible in the requested direction 
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return Color3f(0.0f);

        Normal3f n = its.shFrame.n; // get the normal from the intersection

        // dot(n, -light_dir) gives us how much the normal is aligned with the light direction  
        Color3f color = m_light_color * std::max(0.f, n.dot(-m_light_dir));

        return color;
    }

    std::string toString() const {
        return "NDotLIntegrator[L=" + m_light_dir.toString() + "\n"
               + "\tlight_color=" + m_light_color.toString() + "\n]"; 
    }
protected:
    Vector3f m_light_dir;
    Color3f m_light_color;
};

NORI_REGISTER_CLASS(NDotLIntegrator, "NDotL");
NORI_NAMESPACE_END
