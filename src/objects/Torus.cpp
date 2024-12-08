#include "objects/Torus.h"
#include "objects/Mesh.h"
#include <iostream>

void Torus::drawTriangleMeshFromFaces() {

}

void Torus::drawNormalsFromFaces() {

}

void Torus::draw() {

}

void Torus::drawNormal() {

}

void Torus::calculate() {

}

float Torus::intersect(glm::vec4 rayV, glm::vec4 eyePointP)
{
    // Convert 4D vectors to 3D
    glm::vec3 eyePoint = glm::vec3(eyePointP);
    glm::vec3 ray = glm::vec3(rayV);

    float po = 1.0;  // parity operator

    // Compute square of major (R) and minor (r) radii
    float R_square = Radius * Radius;
    float r_square = radius * radius;

    // Dot products for eyePoint and ray vectors
    float m = dot(eyePoint, eyePoint);
    float n = dot(eyePoint, ray);

    // Bounding sphere check
    {
        // Calculate h, representing part of the discriminant for bounding sphere intersection
        float h = n * n - m + (Radius + radius) * (Radius + radius);
        if (h < 0.0) return -1.0;  // Return -1 if no intersection with bounding sphere
        // float t = -n - sqrt(h); // Can use this to calculate specific intersection points
    }

    // Calculate coefficients for the quartic equation
    float k = (m - r_square - R_square) / 2.0;
    float k3 = n;
    float k2 = n * n + R_square * ray.z * ray.z + k;
    float k1 = k * n + R_square * eyePoint.z * ray.z;
    float k0 = k * k + R_square * eyePoint.z * eyePoint.z - R_square * r_square;

    // Adjust parity operator to prevent |c1| from being too close to zero
    if (abs(k3 * (k3 * k3 - k2) + k1) < 0.01) {
        po = -1.0;  // flip parity operator
        float tmp = k1;
        k1 = k3;
        k3 = tmp;
        k0 = 1.0 / k0;
        k1 = k1 * k0;
        k2 = k2 * k0;
        k3 = k3 * k0;
    }

    // Calculate coefficients of the cubic resolvent
    float c2 = 2.0 * k2 - 3.0 * k3 * k3;
    float c1 = k3 * (k3 * k3 - k2) + k1;
    float c0 = k3 * (k3 * (-3.0 * k3 * k3 + 4.0 * k2) - 8.0 * k1) + 4.0 * k0;

    // Normalize coefficients
    c2 /= 3.0;
    c1 *= 2.0;
    c0 /= 3.0;

    // Calculate discriminants Q and R
    float Q = c2 * c2 + c0;
    float R = 3.0 * c0 * c2 - c2 * c2 * c2 - c1 * c1;

    // Calculate h for determining the number of intersections
    float h = R * R - Q * Q * Q;
    float z = 0.0;

    if (h < 0.0) {
        // Four intersections
        float sQ = sqrt(Q);
        z = 2.0 * sQ * cos(acos(R / (sQ * Q)) / 3.0);
    }
    else {
        // Two intersections
        float sQ = pow(sqrt(h) + abs(R), 1.0 / 3.0);
        float signR = R > 0 ? 1 : -1;
        z = signR * abs(sQ + Q / sQ);
    }

    z = c2 - z;

    // Calculate potential distances d1 and d2
    float d1 = z - 3.0 * c2;
    float d2 = z * z - 3.0 * c0;

    if (abs(d1) < 1.0e-4) {
        if (d2 < 0.0) return -1.0;
        d2 = sqrt(d2);
    }
    else {
        if (d1 < 0.0) return -1.0;
        d1 = sqrt(d1 / 2.0);
        d2 = c1 / d1;
    }

    //----------------------------------
    // Final intersection calculations

    float result = 1e20;  // Initialize result with a large value

    // Check two potential intersections based on discriminants
    h = d1 * d1 - z + d2;
    if (h > 0.0) {
        h = sqrt(h);
        float t1 = -d1 - h - k3;
        t1 = (po < 0.0) ? 2.0 / t1 : t1;
        float t2 = -d1 + h - k3;
        t2 = (po < 0.0) ? 2.0 / t2 : t2;
        if (t1 > 0.0) result = t1;
        if (t2 > 0.0) result = MIN(result, t2);
    }

    h = d1 * d1 - z - d2;
    if (h > 0.0) {
        h = sqrt(h);
        float t1 = d1 - h - k3;
        t1 = (po < 0.0) ? 2.0 / t1 : t1;
        float t2 = d1 + h - k3;
        t2 = (po < 0.0) ? 2.0 / t2 : t2;
        if (t1 > 0.0) result = MIN(result, t1);
        if (t2 > 0.0) result = MIN(result, t2);
    }

    // Return the smallest positive intersection distance or -1 if no valid intersection
    return result;
}

glm::vec3 Torus::getNormal(glm::vec4 rayV, glm::vec4 eyePointP, float t) {
    glm::vec3 pos = eyePointP + t * rayV;
    return normalize(pos * (glm::dot(pos, pos) - radius * radius - Radius * Radius * glm::vec3(1.0, 1.0, -1.0)));
}
