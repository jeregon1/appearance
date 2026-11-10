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
#include <nori/vector.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN
// Hecho
Point2f Warp::squareToUniformSquare(const Point2f &sample) {
    return sample;
}
// Hecho
float Warp::squareToUniformSquarePdf(const Point2f &sample) {
    return ((sample.array() >= 0).all() && (sample.array() <= 1).all()) ? 1.0f : 0.0f;
}

// Falta
Point2f Warp::squareToTent(const Point2f &sample) {
    throw NoriException("Warp::squareToTent() is not yet implemented!");
}
// Falta
float Warp::squareToTentPdf(const Point2f &p) {
    throw NoriException("Warp::squareToTentPdf() is not yet implemented!");
}

// Hecho
Point2f Warp::squareToUniformDisk(const Point2f &sample) {
    // throw NoriException("Warp::squareToUniformDisk() is not yet implemented!");
    float chi = 2*M_PI*sample.y();
    float r = std::sqrt(sample.x());

    float x = r*std::cos(chi);
    float y = r*std::sin(chi);
    return Point2f(x,y);
}
// Hecho
float Warp::squareToUniformDiskPdf(const Point2f &p) {
    // throw NoriException("Warp::squareToUniformDiskPdf() is not yet implemented!");
    return (p.x() * p.x() + p.y() * p.y() <= 1.0f) ? (1.0f / M_PI) : 0.0f;
}

// Hecho
Point2f Warp::squareToUniformTriangle(const Point2f& sample) {
    // throw NoriException("Warp::squareToUniformTriangle() is not yet implemented!");
    float u = 1 - std::sqrt(sample.x());
    float v = sample.y()*std::sqrt(sample.x());
    return Point2f(u,v);
}
// Hecho
float Warp::squareToUniformTrianglePdf(const Point2f& p) {
    // throw NoriException("Warp::squareToUniformTrianglePdf() is not yet implemented!");
    return (p.x() >= 0.0f && p.y() >= 0.0f && p.x() + p.y() <= 1.0f) ? 2.0f : 0.0f;
}

// Hecho
Vector3f Warp::squareToUniformSphere(const Point2f &sample) {
    // throw NoriException("Warp::squareToUniformSphere() is not yet implemented!");
    float z = 1.0f - 2.0f * sample.x();
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2.0f * M_PI * sample.y();
    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}
// Hecho
float Warp::squareToUniformSpherePdf(const Vector3f &v) {
    // throw NoriException("Warp::squareToUniformSpherePdf() is not yet implemented!");
    return 1.0f / (4.0f * M_PI);
}

// Hecho
Vector3f Warp::squareToUniformHemisphere(const Point2f &sample) {
    // throw NoriException("Warp::squareToUniformHemisphere() is not yet implemented!");
    float z = sample.x();
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2.0f * M_PI * sample.y();
    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}
// Hecho
float Warp::squareToUniformHemispherePdf(const Vector3f &v) {
    // throw NoriException("Warp::squareToUniformHemispherePdf() is not yet implemented!");
    return v.z() >= 0.0f ? 1.0f / (2.0f * M_PI) : 0.0f;
}

// Hecho
Vector3f Warp::squareToCosineHemisphere(const Point2f &sample) {
    // throw NoriException("Warp::squareToCosineHemisphere() is not yet implemented!");
    float r = std::sqrt(sample.x());
    float phi = 2.0f * M_PI * sample.y();
    float x = r * std::cos(phi);
    float y = r * std::sin(phi);
    float z = std::sqrt(std::max(0.0f, 1.0f - x * x - y * y));
    return Vector3f(x, y, z);
}
// Hecho
float Warp::squareToCosineHemispherePdf(const Vector3f &v) {
    // throw NoriException("Warp::squareToCosineHemispherePdf() is not yet implemented!");
    return v.z() >= 0.0f ? v.z() / M_PI : 0.0f;
}

// Hecho
Vector3f Warp::squareToBeckmann(const Point2f &sample, float alpha) {
    // throw NoriException("Warp::squareToBeckmann() is not yet implemented!");
    float phi = 2.0f * M_PI * sample.x();
    float tan2Theta = -alpha * alpha * std::log(1.0f - sample.y());
    float cosTheta = 1.0f / std::sqrt(1.0f + tan2Theta);
    float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    return Vector3f(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
}
// Hecho
float Warp::squareToBeckmannPdf(const Vector3f &m, float alpha) {
    // throw NoriException("Warp::squareToBeckmannPdf() is not yet implemented!");
    if (m.z() <= 0.0f) return 0.0f;
    float cosTheta = m.z();
    float tan2Theta = (1.0f - cosTheta * cosTheta) / (cosTheta * cosTheta);
    float alpha2 = alpha * alpha;
    float d = std::exp(-tan2Theta / alpha2) / (M_PI * alpha2 * std::pow(cosTheta, 4.0f));
    return d * cosTheta;
}


NORI_NAMESPACE_END
