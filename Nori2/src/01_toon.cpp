#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class ToonIntegrator : public Integrator {
public:
    ToonIntegrator(const PropertyList &props) {
        // Leemos las propiedades del XML (o ponemos valores por defecto)
        m_light_dir = props.getVector("light_direction", Vector3f(0.f,-1.f,0.f));
        m_light_dir = m_light_dir.normalized();
        m_light_color = props.getColor("light_color", Color3f(1.f));
        m_ambient_color = props.getColor("ambient_color", Color3f(0.1f));
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return Color3f(0.0f); // Fondo negro

        Normal3f n = its.shFrame.n;
        
        // 1. Calcular la luz base (cel shading de 2 tonos)
        float light = n.dot(-m_light_dir);
        if (light > 0.5f) light = 1.0f; // Tono claro
        else if (light > 0.0f) light = 0.5f; // Tono medio
        else light = 0.0f; // Sombra

        // 2. Calcular los brillos especulares exagerados
        Vector3f view_dir = -ray.d.normalized(); // Dirección hacia la cámara
        Vector3f half_vector = (-m_light_dir + view_dir).normalized(); // Vector a medias entre luz y cámara
        float specular_highlight = std::pow(std::max(0.f, n.dot(half_vector)), 50.f); // Exponente alto = brillo pequeño
        specular_highlight = smoothstep(0.5f, 0.51f, specular_highlight); // Hacer el brillo muy duro

        // 3. Calcular los bordes (rim lighting)
        float edge = 1.0f - std::max(0.f, n.dot(view_dir));
        edge = smoothstep(0.8f, 0.9f, edge); // Ajustar el grosor del borde

        // Combinar luz, brillos, bordes y color ambiente
        Color3f color = m_light_color * (light + specular_highlight + edge) + m_ambient_color;
        
        // Nota extra: la guía sugiere añadir sombras proyectadas aquí (similar a direct_whitted)
        // Ray3f shadow_ray(its.p, -m_light_dir);
        // if (scene->rayIntersect(shadow_ray)) {
        //     color = m_ambient_color; // Si hay sombra, solo dejamos el color ambiente
        // }

        return color;
    }

    std::string toString() const {
        return "ToonIntegrator[]";
    }

protected:
    Vector3f m_light_dir;
    Color3f m_light_color;
    Color3f m_ambient_color;

    // Función auxiliar para interpolación suave (similar a GLSL)
    float smoothstep(float edge0, float edge1, float x) const {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
};

NORI_REGISTER_CLASS(ToonIntegrator, "toon");
NORI_NAMESPACE_END