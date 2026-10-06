// MIT License
// Copyright (c) 2019 Erin Catto
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "box2d/b2_relative_joint.h"
#include "box2d/b2_body.h"
#include "box2d/b2_time_step.h"

b2RelativeJoint::b2RelativeJoint(const b2RelativeJointDef* def)
	: b2Joint(def),
	  m_relativeVelocity(def->relativeVelocity),
	  m_previousLinearVelocityA(def->bodyA->GetLinearVelocity())
{
}

b2Vec2 b2RelativeJoint::GetAnchorA() const
{
	return b2Vec2_zero;
}

b2Vec2 b2RelativeJoint::GetAnchorB() const
{
	return b2Vec2_zero;
}

b2Vec2 b2RelativeJoint::GetReactionForce(float inv_dt) const
{
	B2_NOT_USED(inv_dt);
	return b2Vec2_zero;
}

float b2RelativeJoint::GetReactionTorque(float inv_dt) const
{
	B2_NOT_USED(inv_dt);
	return 0.0f;
}

void b2RelativeJoint::SetRelativeVelocity(const b2Vec2& velocity)
{
	m_relativeVelocity = velocity;
}

const b2Vec2& b2RelativeJoint::GetRelativeVelocity() const
{
	return m_relativeVelocity;
}

void b2RelativeJoint::InitVelocityConstraints(const b2SolverData& data)
{
	B2_NOT_USED(data);
}

void b2RelativeJoint::SolveVelocityConstraints(const b2SolverData& data)
{
	b2Velocity& velocityA = data.velocities[m_bodyA->m_islandIndex];
	b2Velocity& velocityB = data.velocities[m_bodyB->m_islandIndex];
	velocityB.v += velocityA.v - m_previousLinearVelocityA;
	m_previousLinearVelocityA = velocityA.v;
}

bool b2RelativeJoint::SolvePositionConstraints(const b2SolverData& data)
{
	B2_NOT_USED(data);
	return true;
}
