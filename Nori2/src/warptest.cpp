/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/warp.h>
#include <nori/bsdf.h>
#include <nanogui/screen.h>
#include <nanogui/label.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/combobox.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/checkbox.h>
#include <nanogui/messagedialog.h>
#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/shader.h>
#include <nanogui/renderpass.h>
#include <nanogui/opengl.h>
#include <pcg32.h>
#include <hypothesis.h>
#include <tinyformat.h>
#include <Eigen/Core>

/* =======================================================================
 *   WARNING    WARNING    WARNING    WARNING    WARNING    WARNING
 * =======================================================================
 *   Remember to put on SAFETY GOGGLES before looking at this file. You
 *   are most certainly not expected to read or understand any of it.
 * ======================================================================= */

#if defined(_MSC_VER)
#  pragma warning (disable: 4305 4244)
#endif

using namespace nanogui;
using namespace std;

using nori::NoriException;
using nori::NoriObjectFactory;
using nori::Point2f;
using nori::Point3f;
using nori::MatrixXf;
using nori::MatrixXu;
using nori::Warp;
using nori::PropertyList;
using nori::BSDF;
using nori::BSDFQueryRecord;
using nori::Color3f;


enum PointType : int {
    Independent = 0,
    Grid,
    Stratified
};

enum WarpType : int {
    Square = 0,
    Tent,
    Disk,
    Triangle,
    UniformSphere,
    UniformHemisphere,
    CosineHemisphere,
    Beckmann,
    MicrofacetBRDF,
    WarpTypeCount
};
static const std::string kWarpTypeNames[WarpTypeCount] = {
    "square", "tent", "disk", "triangle", "uniform_sphere", "uniform_hemisphere",
    "cosine_hemisphere", "beckmann", "microfacet_brdf"
};

struct WarpTest {
    static const int kDefaultXres = 51;
    static const int kDefaultYres = 51;

    WarpType warpType;
    float parameterValue;
    BSDF *bsdf;
    BSDFQueryRecord bRec;
    int xres, yres, res;

    // Observed and expected frequencies, initialized after calling run().
    std::unique_ptr<double[]> obsFrequencies, expFrequencies;

    WarpTest(WarpType warpType_, float parameterValue_, BSDF *bsdf_ = nullptr,
             BSDFQueryRecord bRec_ = BSDFQueryRecord(nori::Vector3f()),
             int xres_ = kDefaultXres, int yres_ = kDefaultYres)
        : warpType(warpType_), parameterValue(parameterValue_), bsdf(bsdf_),
          bRec(bRec_), xres(xres_), yres(yres_) {

        if (warpType != Square && warpType != Disk && warpType != Tent)
            xres *= 2;
        res = xres * yres;
    }

    std::pair<bool, std::string> run() {
        int sampleCount = 1000 * res;
        obsFrequencies.reset(new double[res]);
        expFrequencies.reset(new double[res]);
        memset(obsFrequencies.get(), 0, res*sizeof(double));
        memset(expFrequencies.get(), 0, res*sizeof(double));

        MatrixXf points, values;
        generatePoints(sampleCount, Independent, points, values);

        for (int i=0; i<sampleCount; ++i) {
            if (values(0, i) == 0)
                continue;
            nori::Vector3f sample = points.col(i);
            float x, y;

            if (warpType == Square || warpType == Triangle) {
                x = sample.x();
                y = sample.y();
            } else if (warpType == Disk || warpType == Tent) {
                x = sample.x() * 0.5f + 0.5f;
                y = sample.y() * 0.5f + 0.5f;
            } else {
                x = std::atan2(sample.y(), sample.x()) * INV_TWOPI;
                if (x < 0)
                    x += 1;
                y = sample.z() * 0.5f + 0.5f;
            }

            int xbin = std::min(xres-1, std::max(0, (int) std::floor(x * xres)));
            int ybin = std::min(yres-1, std::max(0, (int) std::floor(y * yres)));
            obsFrequencies[ybin * xres + xbin] += 1;
        }

        auto integrand = [&](double y, double x) -> double {
            if (warpType == Square) {
                return Warp::squareToUniformSquarePdf(Point2f(x, y));
            } else if (warpType == Disk) {
                x = x * 2 - 1; y = y * 2 - 1;
                return Warp::squareToUniformDiskPdf(Point2f(x, y));
            } else if (warpType == Triangle) {
                return Warp::squareToUniformTrianglePdf(Point2f(x, y));
            } else if (warpType == Tent) {
                x = x * 2 - 1; y = y * 2 - 1;
                return Warp::squareToTentPdf(Point2f(x, y));
            } else {
                x *= 2 * M_PI;
                y = y * 2 - 1;

                double sinTheta = std::sqrt(1 - y * y);
                double sinPhi = std::sin(x),
                       cosPhi = std::cos(x);

                nori::Vector3f v((float) (sinTheta * cosPhi),
                                  (float) (sinTheta * sinPhi),
                                  (float) y);

                if (warpType == UniformSphere)
                    return Warp::squareToUniformSpherePdf(v);
                else if (warpType == UniformHemisphere)
                    return Warp::squareToUniformHemispherePdf(v);
                else if (warpType == CosineHemisphere)
                    return Warp::squareToCosineHemispherePdf(v);
                else if (warpType == Beckmann)
                    return Warp::squareToBeckmannPdf(v, parameterValue);
                else if (warpType == MicrofacetBRDF) {
                    BSDFQueryRecord br(bRec);
                    br.wo = v;
                    br.measure = nori::ESolidAngle;
                    return bsdf->pdf(br);
                } else {
                    throw NoriException("Invalid warp type");
                }
            }
        };

        double scale = sampleCount;
        if (warpType == Square || warpType == Triangle)
            scale *= 1;
        else if (warpType == Disk || warpType == Tent)
            scale *= 4;
        else
            scale *= 4*M_PI;

        double *ptr = expFrequencies.get();
        for (int y=0; y<yres; ++y) {
            double yStart =  y    / (double) yres;
            double yEnd   = (y+1) / (double) yres;
            for (int x=0; x<xres; ++x) {
                double xStart =  x    / (double) xres;
                double xEnd   = (x+1) / (double) xres;
                ptr[y * xres + x] = hypothesis::adaptiveSimpson2D(
                    integrand, yStart, xStart, yEnd, xEnd) * scale;
                if (ptr[y * xres + x] < 0)
                    throw NoriException("The Pdf() function returned negative values!");
            }
        }

        /* Write the test input data to disk for debugging */
        hypothesis::chi2_dump(yres, xres, obsFrequencies.get(), expFrequencies.get(), "chitest.m");

        /* Perform the Chi^2 test */
        const int minExpFrequency = 5;
        const float significanceLevel = 0.01f;

        return hypothesis::chi2_test(yres*xres, obsFrequencies.get(),
                                     expFrequencies.get(), sampleCount,
                                     minExpFrequency, significanceLevel, 1);
    }


    std::pair<Point3f, float> warpPoint(const Point2f &sample) {
        Point3f result;

        switch (warpType) {
            case Square:
                result << Warp::squareToUniformSquare(sample), 0; break;
            case Tent:
                result << Warp::squareToTent(sample), 0; break;
            case Disk:
                result << Warp::squareToUniformDisk(sample), 0; break;
            case Triangle:
                result << Warp::squareToUniformTriangle(sample), 0; break;
            case UniformSphere:
                result << Warp::squareToUniformSphere(sample); break;
            case UniformHemisphere:
                result << Warp::squareToUniformHemisphere(sample); break;
            case CosineHemisphere:
                result << Warp::squareToCosineHemisphere(sample); break;
            case Beckmann:
                result << Warp::squareToBeckmann(sample, parameterValue); break;
            case MicrofacetBRDF: {
                BSDFQueryRecord br(bRec);
                float value = bsdf->sample(br, sample).getLuminance();
                return std::make_pair(
                    br.wo,
                    value == 0 ? 0.f : bsdf->eval(br)[0]
                );
             }
             default:
                throw std::runtime_error("Unsupported warp type.");
        }

        return std::make_pair(result, 1.f);
    }


    void generatePoints(int &pointCount, PointType pointType,
                        MatrixXf &positions, MatrixXf &weights) {
        /* Determine the number of points that should be sampled */
        int sqrtVal = (int) (std::sqrt((float) pointCount) + 0.5f);
        float invSqrtVal = 1.f / sqrtVal;
        if (pointType == Grid || pointType == Stratified)
            pointCount = sqrtVal*sqrtVal;

        pcg32 rng;
        positions.resize(3, pointCount);
        weights.resize(1, pointCount);

        for (int i=0; i<pointCount; ++i) {
            int y = i / sqrtVal, x = i % sqrtVal;
            Point2f sample;

            switch (pointType) {
                case Independent:
                    sample = Point2f(rng.nextFloat(), rng.nextFloat());
                    break;

                case Grid:
                    sample = Point2f((x + 0.5f) * invSqrtVal, (y + 0.5f) * invSqrtVal);
                    break;

                case Stratified:
                    sample = Point2f((x + rng.nextFloat()) * invSqrtVal,
                                     (y + rng.nextFloat()) * invSqrtVal);
                    break;
            }

            auto result = warpPoint(sample);
            positions.col(i) = result.first;
            weights(0, i) = result.second;
        }
    }

    static std::pair<BSDF *, BSDFQueryRecord>
    create_microfacet_bsdf(float alpha, float kd, float bsdfAngle) {
        PropertyList list;
        list.setFloat("alpha", alpha);
        list.setColor("kd", Color3f(kd));
        auto * brdf = (BSDF *) NoriObjectFactory::createInstance("microfacet", list);

        nori::Vector3f wi(std::sin(bsdfAngle), 0.f,
                  std::max(std::cos(bsdfAngle), 1e-4f));
        wi = wi.normalized();
        BSDFQueryRecord bRec(wi);
        return { brdf, bRec };
    }
};

class WarpTestScreen : public Screen {
public:

    WarpTestScreen(): Screen(nanogui::Vector2i(800, 600), "warptest: Sampling and Warping"), m_bRec(nori::Vector3f()) {
        initializeGUI();
        m_drawHistogram = false;
    }

    static float mapParameter(WarpType warpType, float parameterValue) {
        if (warpType == Beckmann || warpType == MicrofacetBRDF)
            parameterValue = std::exp(std::log(0.01f) * (1 - parameterValue) +
                                      std::log(1.f)   *  parameterValue);
        return parameterValue;
    }

    void refresh() {
        PointType pointType = (PointType) m_pointTypeBox->selected_index();
        WarpType warpType = (WarpType) m_warpTypeBox->selected_index();
        float parameterValue = mapParameter(warpType, m_parameterSlider->value());
        float parameter2Value = mapParameter(warpType, m_parameter2Slider->value());
        m_pointCount = (int) std::pow(2.f, 15 * m_pointCountSlider->value() + 5);

        if (warpType == MicrofacetBRDF) {
            BSDF *ptr;
            float bsdfAngle = M_PI * (m_angleSlider->value() - 0.5f);
            std::tie(ptr, m_bRec) = WarpTest::create_microfacet_bsdf(
                parameterValue, parameter2Value, bsdfAngle);
            m_brdf.reset(ptr);
        }

        /* Generate the point positions */
        MatrixXf positions, values;
        try {
            WarpTest tester(warpType, parameterValue, m_brdf.get(), m_bRec);
            tester.generatePoints(m_pointCount, pointType, positions, values);
        } catch (const NoriException &e) {
            m_warpTypeBox->set_selected_index(0);
            refresh();
            new MessageDialog(this, MessageDialog::Type::Warning, "Error", "An error occurred: " + std::string(e.what()));
            return;
        }

        float value_scale = 0.f;
        for (int i=0; i<m_pointCount; ++i)
            value_scale = std::max(value_scale, values(0, i));
        value_scale = 1.f/value_scale;

        if (!m_brdfValueCheckBox->checked() || warpType != MicrofacetBRDF)
            value_scale = 0.f;

        if (warpType != Square) {
            for (int i=0; i<m_pointCount; ++i) {
                if (values(0, i) == 0.0f) {
                    positions.col(i) = nori::Vector3f::Constant(std::numeric_limits<float>::quiet_NaN());
                    continue;
                }
                positions.col(i) =
                    ((value_scale == 0 ? 1.0f : (value_scale * values(0, i))) *
                     positions.col(i)) * 0.5f + nori::Vector3f(0.5f, 0.5f, 0.0f);
            }
        }

        /* Generate a color gradient */
        MatrixXf colors(3, m_pointCount);
        float colScale = 1.f / m_pointCount;
        for (int i=0; i<m_pointCount; ++i)
            colors.col(i) << i*colScale, 1-i*colScale, 0;

        /* Upload points to GPU */
        m_pointShader->set_buffer("position", nanogui::VariableType::Float32,
                      { (size_t) m_pointCount, 3 }, positions.data());
        m_pointShader->set_buffer("color", nanogui::VariableType::Float32,
                      { (size_t) m_pointCount, 3 }, colors.data());

        /* Upload lines to GPU */
        if (m_gridCheckBox->checked()) {
            int gridRes = (int) (std::sqrt((float) m_pointCount) + 0.5f);
            int fineGridRes = 16*gridRes, idx = 0;
            m_lineCount = 4 * (gridRes+1) * (fineGridRes+1);
            positions.resize(3, m_lineCount);
            float coarseScale = 1.f / gridRes, fineScale = 1.f / fineGridRes;

            WarpTest tester(warpType, parameterValue, m_brdf.get(), m_bRec);
            for (int i=0; i<=gridRes; ++i) {
                for (int j=0; j<=fineGridRes; ++j) {
                    auto pt = tester.warpPoint(Point2f(j * fineScale, i * coarseScale));
                    positions.col(idx++) = value_scale == 0.f ? pt.first : (pt.first * pt.second * value_scale);
                    pt = tester.warpPoint(Point2f((j+1) * fineScale, i * coarseScale));
                    positions.col(idx++) = value_scale == 0.f ? pt.first : (pt.first * pt.second * value_scale);
                    pt = tester.warpPoint(Point2f(i*coarseScale, j     * fineScale));
                    positions.col(idx++) = value_scale == 0.f ? pt.first : (pt.first * pt.second * value_scale);
                    pt = tester.warpPoint(Point2f(i*coarseScale, (j+1) * fineScale));
                    positions.col(idx++) = value_scale == 0.f ? pt.first : (pt.first * pt.second * value_scale);
                }
            }
            if (warpType != Square) {
                for (int i=0; i<m_lineCount; ++i)
                    positions.col(i) = positions.col(i) * 0.5f + nori::Vector3f(0.5f, 0.5f, 0.0f);
            }
            m_gridShader->set_buffer("position", nanogui::VariableType::Float32,
                                     { (size_t) m_lineCount, 3 }, positions.data());
        }

        int ctr = 0;
        positions.resize(3, 106);
        for (int i=0; i<=50; ++i) {
            float angle1 = i * 2 * M_PI / 50;
            float angle2 = (i+1) * 2 * M_PI / 50;
            positions.col(ctr++) << std::cos(angle1)*.5f + 0.5f, std::sin(angle1)*.5f + 0.5f, 0.f;
            positions.col(ctr++) << std::cos(angle2)*.5f + 0.5f, std::sin(angle2)*.5f + 0.5f, 0.f;
        }
        positions.col(ctr++) << 0.5f, 0.5f, 0.f;
        positions.col(ctr++) << -m_bRec.wi.x() * 0.5f + 0.5f, -m_bRec.wi.y() * 0.5f + 0.5f, m_bRec.wi.z() * 0.5f;
        positions.col(ctr++) << 0.5f, 0.5f, 0.f;
        positions.col(ctr++) << m_bRec.wi.x() * 0.5f + 0.5f, m_bRec.wi.y() * 0.5f + 0.5f, m_bRec.wi.z() * 0.5f;
        m_arrowShader->set_buffer("position", nanogui::VariableType::Float32,
                      { 106, 3 }, positions.data());

        /* Update user interface */
        std::string str;
        if (m_pointCount > 1000000) {
            m_pointCountBox->set_units("M");
            str = tfm::format("%.2f", m_pointCount * 1e-6f);
        } else if (m_pointCount > 1000) {
            m_pointCountBox->set_units("K");
            str = tfm::format("%.2f", m_pointCount * 1e-3f);
        } else {
            m_pointCountBox->set_units(" ");
            str = tfm::format("%i", m_pointCount);
        }
        m_pointCountBox->set_value(str);
        m_parameterBox->set_value(tfm::format("%.1g", parameterValue));
        m_parameter2Box->set_value(tfm::format("%.1g", parameter2Value));
        m_angleBox->set_value(tfm::format("%.1f", m_angleSlider->value() * 180-90));
        m_parameterSlider->set_enabled(warpType == Beckmann || warpType == MicrofacetBRDF);
        m_parameterBox->set_enabled(warpType == Beckmann || warpType == MicrofacetBRDF);
        m_parameter2Slider->set_enabled(warpType == MicrofacetBRDF);
        m_parameter2Box->set_enabled(warpType == MicrofacetBRDF);
        m_angleBox->set_enabled(warpType == MicrofacetBRDF);
        m_angleSlider->set_enabled(warpType == MicrofacetBRDF);
        m_brdfValueCheckBox->set_enabled(warpType == MicrofacetBRDF);
        m_pointCountSlider->set_value((std::log((float) m_pointCount) / std::log(2.f) - 5) / 15);
    }

    ~WarpTestScreen() {
        glDeleteTextures(2, &m_textures[0]);
        delete m_pointShader;
        delete m_gridShader;
        delete m_arrowShader;
        delete m_histogramShader;
        delete m_renderPass;
    }

    bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) {
        if (!Screen::mouse_motion_event(p, rel, button, modifiers) && m_rotating) {
            m_rotation = nanogui::Matrix4f::rotate(nanogui::Vector3f(0.f, 1.f, 0.f), rel.x() * 0.01f) *
                         nanogui::Matrix4f::rotate(nanogui::Vector3f(1.f, 0.f, 0.f), rel.y() * 0.01f) * m_rotation;
        }
        return true;
    }

    bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
        if (down && !m_window->visible()) {
            m_drawHistogram = false;
            m_window->set_visible(true);
            return true;
        }
        if (!Screen::mouse_button_event(p, button, down, modifiers)) {
            if (button == GLFW_MOUSE_BUTTON_1)
                m_rotating = down;
        }
        return true;
    }

    void drawContents() {
        /* Set up a perspective camera matrix */
        nanogui::Matrix4f view, proj, model;
        view = nanogui::Matrix4f::look_at(nanogui::Vector3f(0, 0, 2), nanogui::Vector3f(0, 0, 0), nanogui::Vector3f(0, 1, 0));
        const float viewAngle = 30, near_clip = 0.01, far_clip = 100;
        float fH = std::tan(viewAngle / 360.0f * M_PI) * near_clip;
        float fW = fH * (float) size().x() / (float) size().y();
        proj = nanogui::Matrix4f::perspective(viewAngle * (float) M_PI / 180.f,
                               near_clip, far_clip, fW / fH);

        model = m_rotation * nanogui::Matrix4f::translate(nanogui::Vector3f(-0.5f, -0.5f, 0.0f));

        m_renderPass->resize(framebuffer_size());
        m_renderPass->begin();

        if (m_drawHistogram) {
            /* Render the histograms */
            WarpType warpType = (WarpType) m_warpTypeBox->selected_index();
            const int spacer = 20;
            const int histWidth = (width() - 3*spacer) / 2;
            const int histHeight = (warpType == Square || warpType == Disk || warpType == Triangle || warpType == Tent) ? histWidth : histWidth / 2;
            const int verticalOffset = (height() - histHeight) / 2;

            // 2025: the Axis Labels Update
            const bool isSpherical = (warpType == UniformSphere ||
                                      warpType == UniformHemisphere ||
                                      warpType == CosineHemisphere ||
                                      warpType == Beckmann ||
                                      warpType == MicrofacetBRDF);
            // // Units 
            std::string textU = (!isSpherical) ? "X" : "Φ";
            std::string textV = (!isSpherical) ? "Y" : "θ";
            // // Ranges
            std::string minU = "0", maxU = "1";
            std::string minV = "0", maxV = "1";
            if (isSpherical) {
                minU = "0"; maxU = "2π";
                minV = "π"; maxV = "0";
            }
            else if (warpType == Disk || warpType == Tent) {
                minU = "-1"; maxU = "1";
                minV = "-1"; maxV = "1";
            }

            // ---------------------------------

            drawHistogram(nanogui::Vector2i(spacer, verticalOffset), nanogui::Vector2i(histWidth, histHeight), m_textures[0]);
            drawHistogram(nanogui::Vector2i(2*spacer + histWidth, verticalOffset), nanogui::Vector2i(histWidth, histHeight), m_textures[1]);

            auto ctx = nvg_context();
            nvgBeginFrame(ctx, size()[0], size()[1], pixel_ratio());
            nvgBeginPath(ctx);
            nvgRect(ctx, spacer, verticalOffset + histHeight + spacer, width()-2*spacer, 70);
            nvgFillColor(ctx, m_testResult.first ? Color(100, 255, 100, 100) : Color(255, 100, 100, 100));
            nvgFill(ctx);
            nvgFontSize(ctx, 24.0f);
            nvgFontFace(ctx, "sans-bold");
            nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgFillColor(ctx, Color(255, 255));
            nvgText(ctx, spacer + histWidth / 2, verticalOffset - 3 * spacer,
                    "Sample histogram", nullptr);
            nvgText(ctx, 2 * spacer + (histWidth * 3) / 2, verticalOffset - 3 * spacer,
                    "Integrated density", nullptr);

            // ---------------------------------
            // new 2025: 
            // Axis labels
            nvgFontSize(ctx, 19.0f);
            nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(ctx, spacer + histWidth / 2, verticalOffset + histHeight + spacer / 10.f,
                    textU.c_str(), nullptr);
            nvgText(ctx, 2 * spacer + (histWidth * 3) / 2, verticalOffset + histHeight + spacer / 10.f,
                    textU.c_str(), nullptr);
            nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(ctx, spacer*.9f, verticalOffset + histHeight / 2,
                    textV.c_str(), nullptr);
            // nvgText(ctx, spacer / 2, verticalOffset + histHeight / 2,
            //         textV.c_str(), nullptr);
            // Ranges
            nvgFontSize(ctx, 15.0f);
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgText(ctx, spacer, verticalOffset + histHeight + spacer - 15,
                    minU.c_str(), nullptr);
            nvgText(ctx, 2 * spacer + histWidth, verticalOffset + histHeight + spacer - 15,
                    minU.c_str(), nullptr); 
            nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
            nvgText(ctx, spacer + histWidth, verticalOffset + histHeight + spacer - 15,
                    maxU.c_str(), nullptr);
            nvgText(ctx, 2 * spacer + 2 * histWidth, verticalOffset + histHeight + spacer - 15,
                    maxU.c_str(), nullptr);
            nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
            nvgText(ctx, spacer - 5, verticalOffset + histHeight,
                    maxV.c_str(), nullptr); 
            nvgText(ctx, spacer - 5, verticalOffset + histHeight,
                    maxV.c_str(), nullptr);
            nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
            nvgText(ctx, spacer - 5, verticalOffset,
                    minV.c_str(), nullptr); 
            nvgText(ctx, spacer - 5, verticalOffset,
                    minV.c_str(), nullptr);            

            // --------------------------------
            nvgStrokeColor(ctx, Color(255, 255));
            nvgStrokeWidth(ctx, 2);
            nvgBeginPath(ctx);
            nvgRect(ctx, spacer, verticalOffset, histWidth, histHeight);
            nvgRect(ctx, 2 * spacer + histWidth, verticalOffset, histWidth,
                    histHeight);
            nvgStroke(ctx);
            nvgFontSize(ctx, 20.0f);
            nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);

            float bounds[4];
            nvgTextBoxBounds(ctx, 0, 0, width() - 2 * spacer,
                             m_testResult.second.c_str(), nullptr, bounds);
            nvgTextBox(
                ctx, spacer, verticalOffset + histHeight + spacer + (70 - bounds[3])/2,
                width() - 2 * spacer, m_testResult.second.c_str(), nullptr);
            nvgEndFrame(ctx);
        } else {
            /* Render the point set */
            nanogui::Matrix4f mvp = proj * view * model;
            m_pointShader->set_uniform("mvp", mvp);
            m_pointShader->begin();
            m_pointShader->draw_array(nanogui::Shader::PrimitiveType::Point, 0, m_pointCount);
            m_pointShader->end();

            bool drawGrid = m_gridCheckBox->checked();
            if (drawGrid) {
                m_gridShader->set_uniform("mvp", mvp);
                m_gridShader->begin();
                m_gridShader->draw_array(nanogui::Shader::PrimitiveType::Line, 0, m_lineCount);
                m_gridShader->end();
            }
            if (m_warpTypeBox->selected_index() == MicrofacetBRDF) {
                m_arrowShader->set_uniform("mvp", mvp);
                m_arrowShader->begin();
                m_arrowShader->draw_array(nanogui::Shader::PrimitiveType::Line, 0, 106);
                m_arrowShader->end();
            }
        }
        m_renderPass->end();
    }

    void drawHistogram(const nanogui::Vector2i &pos_, const nanogui::Vector2i &size_, GLuint tex) {
        nanogui::Vector2f s(-(pos_.x() + 0.25f) / size_.x(),
                             -(pos_.y() + 0.25f) / size_.y());
        nanogui::Vector2f e(size().x() / (float) size_.x() + s.x(),
                             size().y() / (float) size_.y() + s.y());
        nanogui::Matrix4f mvp = nanogui::Matrix4f::ortho(s.x(), e.x(), e.y(), s.y(), -1, 1);

        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        m_histogramShader->set_uniform("mvp", mvp);
        m_histogramShader->set_uniform("tex", 0);
        m_histogramShader->begin();
        m_histogramShader->draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, 6, true);
        m_histogramShader->end();
    }

    void runTest() {
        // Prepare and run test, passing parameters from UI.
        WarpType warpType = (WarpType) m_warpTypeBox->selected_index();
        float parameterValue = mapParameter(warpType, m_parameterSlider->value());

        WarpTest tester(warpType, parameterValue, m_brdf.get(), m_bRec);
        m_testResult = tester.run();

        float maxValue = 0, minValue = std::numeric_limits<float>::infinity();
        for (int i=0; i<tester.res; ++i) {
            maxValue = std::max(maxValue,
                                (float) std::max(tester.obsFrequencies[i], tester.expFrequencies[i]));
            minValue = std::min(minValue,
                                (float) std::min(tester.obsFrequencies[i], tester.expFrequencies[i]));
        }
        minValue /= 2;
        float texScale = 1/(maxValue - minValue);

        /* Upload histograms to GPU */
        std::unique_ptr<float[]> buffer(new float[tester.res]);
        for (int k=0; k<2; ++k) {
            for (int i=0; i<tester.res; ++i)
                buffer[i] = ((k == 0 ? tester.obsFrequencies[i]
                                     : tester.expFrequencies[i]) - minValue) * texScale;

            glBindTexture(GL_TEXTURE_2D, m_textures[k]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, tester.xres, tester.yres,
                         0, GL_RED, GL_FLOAT, buffer.get());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        m_drawHistogram = true;
        m_window->set_visible(false);
    }

    void initializeGUI() {
        m_window = new Window(this, "Warp tester");
        m_window->set_position(Vector2i(15, 15));
        m_window->set_layout(new GroupLayout());

        new Label(m_window, "Input point set", "sans-bold");

        /* Create an empty panel with a horizontal layout */
        Widget *panel = new Widget(m_window);
        panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));

        /* Add a slider and set defaults */
        m_pointCountSlider = new Slider(panel);
        m_pointCountSlider->set_fixed_width(55);
        m_pointCountSlider->set_callback([&](float) { refresh(); });

        /* Add a textbox and set defaults */
        m_pointCountBox = new TextBox(panel);
        m_pointCountBox->set_fixed_size(Vector2i(80, 25));

        m_pointTypeBox = new ComboBox(m_window, { "Independent", "Grid", "Stratified" });
        m_pointTypeBox->set_callback([&](int) { refresh(); });

        new Label(m_window, "Warping method", "sans-bold");
        m_warpTypeBox = new ComboBox(m_window, { "Square", "Tent", "Disk", "Triangle", "Sphere", "Hemisphere (unif.)",
                "Hemisphere (cos)", "Beckmann distr.", "Microfacet BRDF" });
        m_warpTypeBox->set_callback([&](int) { refresh(); });

        panel = new Widget(m_window);
        panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));
        m_parameterSlider = new Slider(panel);
        m_parameterSlider->set_fixed_width(55);
        m_parameterSlider->set_callback([&](float) { refresh(); });
        m_parameterBox = new TextBox(panel);
        m_parameterBox->set_fixed_size(Vector2i(80, 25));
        panel = new Widget(m_window);
        panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));
        m_parameter2Slider = new Slider(panel);
        m_parameter2Slider->set_fixed_width(55);
        m_parameter2Slider->set_callback([&](float) { refresh(); });
        m_parameter2Box = new TextBox(panel);
        m_parameter2Box->set_fixed_size(Vector2i(80, 25));
        m_gridCheckBox = new CheckBox(m_window, "Visualize warped grid");
        m_gridCheckBox->set_callback([&](bool) { refresh(); });

        new Label(m_window, "BSDF parameters", "sans-bold");

        panel = new Widget(m_window);
        panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));

        m_angleSlider = new Slider(panel);
        m_angleSlider->set_fixed_width(55);
        m_angleSlider->set_callback([&](float) { refresh(); });
        m_angleBox = new TextBox(panel);
        m_angleBox->set_fixed_size(Vector2i(80, 25));
        m_angleBox->set_units(utf8(0x00B0).data());

        m_brdfValueCheckBox = new CheckBox(m_window, "Visualize BRDF values");
        m_brdfValueCheckBox->set_callback([&](bool) { refresh(); });

        new Label(m_window,
            std::string(utf8(0x03C7).data()) +
            std::string(utf8(0x00B2).data()) + " hypothesis test",
            "sans-bold");

        Button *testBtn = new Button(m_window, "Run", FA_CHECK);
        testBtn->set_background_color(Color(0, 255, 0, 25));
        testBtn->set_callback([&]{
            try {
                runTest();
            } catch (const NoriException &e) {
                new MessageDialog(this, MessageDialog::Type::Warning, "Error", "An error occurred: " + std::string(e.what()));
            }
        });

        perform_layout();

        m_renderPass = new nanogui::RenderPass({ this });

        m_pointShader = new nanogui::Shader(
            m_renderPass,
            "Point shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 mvp;\n"
            "in vec3 position;\n"
            "in vec3 color;\n"
            "out vec3 frag_color;\n"
            "void main() {\n"
            "    gl_Position = mvp * vec4(position, 1.0);\n"
            "    if (isnan(position.r)) /* nan (missing value) */\n"
            "        frag_color = vec3(0.0);\n"
            "    else\n"
            "        frag_color = color;\n"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "in vec3 frag_color;\n"
            "out vec4 out_color;\n"
            "void main() {\n"
            "    if (frag_color == vec3(0.0))\n"
            "        discard;\n"
            "    out_color = vec4(frag_color, 1.0);\n"
            "}"
        );

        m_gridShader = new nanogui::Shader(
            m_renderPass,
            "Grid shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 mvp;\n"
            "in vec3 position;\n"
            "void main() {\n"
            "    gl_Position = mvp * vec4(position, 1.0);\n"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "out vec4 out_color;\n"
            "void main() {\n"
            "    out_color = vec4(vec3(1.0), 0.4);\n"
            "}"
        );

        m_arrowShader = new nanogui::Shader(
            m_renderPass,
            "Arrow shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 mvp;\n"
            "in vec3 position;\n"
            "void main() {\n"
            "    gl_Position = mvp * vec4(position, 1.0);\n"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "out vec4 out_color;\n"
            "void main() {\n"
            "    out_color = vec4(vec3(1.0), 0.4);\n"
            "}"
        );

        m_histogramShader = new nanogui::Shader(
            m_renderPass,
            "Histogram shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 mvp;\n"
            "in vec2 position;\n"
            "out vec2 uv;\n"
            "void main() {\n"
            "    gl_Position = mvp * vec4(position, 0.0, 1.0);\n"
            "    uv = position;\n"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "out vec4 out_color;\n"
            "uniform sampler2D tex;\n"
            "in vec2 uv;\n"
            "/* http://paulbourke.net/texture_colour/colourspace/ */\n"
            "vec3 colormap(float v, float vmin, float vmax) {\n"
            "    vec3 c = vec3(1.0);\n"
            "    if (v < vmin)\n"
            "        v = vmin;\n"
            "    if (v > vmax)\n"
            "        v = vmax;\n"
            "    float dv = vmax - vmin;\n"
            "    \n"
            "    if (v < (vmin + 0.25 * dv)) {\n"
            "        c.r = 0.0;\n"
            "        c.g = 4.0 * (v - vmin) / dv;\n"
            "    } else if (v < (vmin + 0.5 * dv)) {\n"
            "        c.r = 0.0;\n"
            "        c.b = 1.0 + 4.0 * (vmin + 0.25 * dv - v) / dv;\n"
            "    } else if (v < (vmin + 0.75 * dv)) {\n"
            "        c.r = 4.0 * (v - vmin - 0.5 * dv) / dv;\n"
            "        c.b = 0.0;\n"
            "    } else {\n"
            "        c.g = 1.0 + 4.0 * (vmin + 0.75 * dv - v) / dv;\n"
            "        c.b = 0.0;\n"
            "    }\n"
            "    return c;\n"
            "}\n"
            "void main() {\n"
            "    float value = texture(tex, uv).r;\n"
            "    out_color = vec4(colormap(value, 0.0, 1.0), 1.0);\n"
            "}"
        );

        /* Upload a single quad */
        MatrixXf positions(2, 4);
        MatrixXu indices(3, 2);
        positions <<
            0, 1, 1, 0,
            0, 0, 1, 1;
        indices <<
            0, 2,
            1, 3,
            2, 0;
        m_histogramShader->set_buffer("position", nanogui::VariableType::Float32,
                          { 4, 2 }, positions.data());
        m_histogramShader->set_buffer("indices", nanogui::VariableType::UInt32,
                          { 6 }, indices.data());

        glGenTextures(2, &m_textures[0]);
        glBindTexture(GL_TEXTURE_2D, m_textures[0]);

        set_background(nanogui::Color(0, 0, 0, 255));
        draw_all();

        /* Set default and register slider callback */
        m_pointCountSlider->set_value(7.f/15.f);
        m_parameterSlider->set_value(.5f);
        m_parameter2Slider->set_value(0.f);
        m_angleSlider->set_value(.5f);

        refresh();
        set_visible(true);
    }
private:
    nanogui::Shader *m_pointShader = nullptr;
    nanogui::Shader *m_gridShader = nullptr;
    nanogui::Shader *m_histogramShader = nullptr;
    nanogui::Shader *m_arrowShader = nullptr;
    nanogui::RenderPass *m_renderPass = nullptr;
    Window *m_window;
    Slider *m_pointCountSlider, *m_parameterSlider, *m_parameter2Slider, *m_angleSlider;
    TextBox *m_pointCountBox, *m_parameterBox, *m_parameter2Box, *m_angleBox;
    GLuint m_textures[2];
    ComboBox *m_pointTypeBox;
    ComboBox *m_warpTypeBox;
    CheckBox *m_gridCheckBox;
    CheckBox *m_brdfValueCheckBox;
    nanogui::Matrix4f m_rotation = nanogui::Matrix4f(1.f);
    bool m_rotating = false;
    int m_pointCount, m_lineCount;
    bool m_drawHistogram;
    std::unique_ptr<BSDF> m_brdf;
    BSDFQueryRecord m_bRec;
    std::pair<bool, std::string> m_testResult;
};


std::tuple<WarpType, float, float> parse_arguments(int argc, char **argv) {
    WarpType tp = WarpTypeCount;
    for (int i = 0; i < WarpTypeCount; ++i) {
        if (strcmp(kWarpTypeNames[i].c_str(), argv[1]) == 0)
            tp = WarpType(i);
    }
    if (tp >= WarpTypeCount)
        throw std::runtime_error("Invalid warp type!");

    float value = 0.f, value2 = 0.f;
    if (argc > 2)
        value = std::stof(argv[2]);
    if (argc > 3)
        value2 = std::stof(argv[3]);

    return { tp, value, value2 };
}


int main(int argc, char **argv) {
    if (argc <= 1) {
        // GUI mode
        nanogui::init();
        WarpTestScreen *screen = new WarpTestScreen();
        nanogui::run();
        delete screen;
        nanogui::shutdown();
        return 0;
    }

    // CLI mode
    WarpType warpType;
    float paramValue, param2Value;
    std::unique_ptr<BSDF> bsdf;
    auto bRec = BSDFQueryRecord(nori::Vector3f());
    std::tie(warpType, paramValue, param2Value) = parse_arguments(argc, argv);
    if (warpType == MicrofacetBRDF) {
        float bsdfAngle = M_PI * 0.f;
        BSDF *ptr;
        std::tie(ptr, bRec) = WarpTest::create_microfacet_bsdf(
            paramValue, param2Value, bsdfAngle);
        bsdf.reset(ptr);
    }

    std::string extra = "";
    if (param2Value > 0)
        extra = tfm::format(", second parameter value = %f", param2Value);
    std::cout << tfm::format(
        "Testing warp %s, parameter value = %f%s",
         kWarpTypeNames[int(warpType)], paramValue, extra
    ) << std::endl;
    WarpTest tester(warpType, paramValue, bsdf.get(), bRec);
    auto res = tester.run();
    if (res.first)
        return 0;

    std::cout << tfm::format("warptest failed: %s", res.second) << std::endl;
    return 1;
}
