#define SECRET_TEACHER_CODE


#ifndef SECRET_TEACHER_CODE
/* Example toon shader showing some operations with normals and vectors
 * Based on https://roystan.net/articles/toon-shader/
*/

#include <nori/integrator.h>
#include <nori/scene.h>
 
NORI_NAMESPACE_BEGIN

// Smoothstep is widely used in computer graphics for smooth blending
// https://en.wikipedia.org/wiki/Smoothstep
// it returns 0 if x <= edge0,  1 if x >= edge1,   
// and a smooth interpolation in between
float smoothstep (float edge0, float edge1, float x) {
    // Scale, and clamp x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return x * x * (3.0f - 2.0f * x);
 }
 

class ToonIntegrator : public Integrator {
public:
    ToonIntegrator(const PropertyList &props) {
        // Read the properties from the xml, or set some defaults:
        m_light_dir = props.getVector("light_direction", Vector3f(0.f,-1.f,0.f));
        m_light_dir = m_light_dir.normalized(); // direction vectors should be normalized
        m_light_color = props.getColor("light_color", Color3f(1.f)); 
        m_ambient_color = props.getColor("ambient_color", Color3f(.05f,.02f,.2f)); 
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        // TODO: remove the 0.f* in front of some terms to enable them

        // Find the surface that is visible in the requested direction 
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return Color3f(0.0f);

        Normal3f n = its.shFrame.n; // get the normal from the intersection

        float ndotl = n.dot(-m_light_dir);

        // toon makes it either fully lit or fully shadowed:
        float light = smoothstep(0.f,0.05f, ndotl); // with a smooth but still sharp transition

        // Specular highlights. Find how much the normal is aligned with the half vector towards the light
        Vector3f half = (-ray.d - m_light_dir).normalized();
        float ndoth = n.dot(half);  
        float specular_highlight = pow(ndoth * light, 80.f); // the highlight only happens when the vectors are very aligned
        specular_highlight = 0.f*smoothstep(0.8f, 0.9f, specular_highlight); // make it a line instead of a gradient    

        // Edges are the places where the normal is close to perpendicular to the view direction:
        float edge = n.dot(ray.d);
        edge = pow(edge*edge, 2.f); // 0 only when the vectors are very perpendicular
        edge = smoothstep(0.005f, 0.01f, edge); // make it a line instead of a gradient
        edge = 1.f - edge; // invert it, so that edges are 1 and other places are 0
        edge = 0.f*edge * light; // only show edges on the lit side

        // Combine light, specular highlights, edges and some ambient color:  
        Color3f color = m_light_color*(light+specular_highlight+edge) + m_ambient_color;
        return color;
    }

    std::string toString() const {
        return "ToonIntegrator[L=" + m_light_dir.toString() + "\n"
               + "\tlight_color=" + m_light_color.toString() + "\n]"; 
    }
protected:
    Vector3f m_light_dir;
    Color3f m_light_color, m_ambient_color;
};

NORI_REGISTER_CLASS(ToonIntegrator, "toon");
NORI_NAMESPACE_END

#else

/* Example toon shader showing some operations with normals and vectors
 * Based on https://roystan.net/articles/toon-shader/
*/

#include <nori/integrator.h>
#include <nori/scene.h>
 
NORI_NAMESPACE_BEGIN

// Smoothstep is widely used in computer graphics for smooth blending
// https://en.wikipedia.org/wiki/Smoothstep
// it returns 0 if x <= edge0,  1 if x >= edge1,   
// and a smooth interpolation in between
float smoothstep (float edge0, float edge1, float x) {
    // Scale, and clamp x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return x * x * (3.0f - 2.0f * x);
 }
 

class ToonIntegrator : public Integrator {
public:
    ToonIntegrator(const PropertyList &props) {
        // Read the properties from the xml, or set some defaults:
        m_light_dir = props.getVector("light_direction", Vector3f(0.f,-1.f,0.f));
        m_light_dir = m_light_dir.normalized(); // direction vectors should be normalized
        m_light_color = props.getColor("light_color", Color3f(1.f)); 
        m_ambient_color = props.getColor("ambient_color", Color3f(.05f,.02f,.2f)); 
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        // TODO: remove the 0.f* in front of some terms to enable them

        // Find the surface that is visible in the requested direction 
        Intersection its;
        if (!scene->rayIntersect(ray, its)) 
            return scene->getBackground(ray); // no intersection

        Normal3f n = its.shFrame.n; // get the normal from the intersection

        float ndotl = n.dot(-m_light_dir);

        // toon makes it either fully lit or fully shadowed:
        float light = smoothstep(0.f,0.05f, ndotl); // with a smooth but still sharp transition

        // Specular highlights. Find how much the normal is aligned with the half vector towards the light
        Vector3f half = (-ray.d - m_light_dir).normalized();
        float ndoth = n.dot(half);  
        float specular_highlight = pow(ndoth * light, 80.f); // the highlight only happens when the vectors are very aligned
        specular_highlight = smoothstep(0.8f, 0.9f, specular_highlight); // make it a line instead of a gradient    

        // Edges are the places where the normal is close to perpendicular to the view direction:
        float edge = n.dot(ray.d);
        edge = pow(edge*edge, 2.f); // 0 only when the vectors are very perpendicular
        edge = smoothstep(0.005f, 0.01f, edge); // make it a line instead of a gradient
        edge = 1.f - edge; // invert it, so that edges are 1 and other places are 0
        edge = edge * light; // only show edges on the lit side

        // Combine light, specular highlights, edges and some ambient color:  
        Color3f color = m_light_color*(light+specular_highlight+edge) + m_ambient_color;


        Ray3f shadow_ray(its.p, -m_light_dir);
        
        if (scene->rayIntersect(shadow_ray))
            color = m_ambient_color; // in shadow, only ambient light

        return color;
    }

    std::string toString() const {
        return "ToonIntegrator[L=" + m_light_dir.toString() + "\n"
               + "\tlight_color=" + m_light_color.toString() + "\n]"; 
    }
protected:
    Vector3f m_light_dir;
    Color3f m_light_color, m_ambient_color;
};

NORI_REGISTER_CLASS(ToonIntegrator, "toon");
NORI_NAMESPACE_END


#endif