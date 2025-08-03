#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
Matrix4x4 MatrixMultiply(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);
float EaseInOut(float t);