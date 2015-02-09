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

#pragma once

#include <nori/proplist.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Base class of all objects
 *
 * A Nori object represents an instance that is part of
 * a scene description, e.g. a scattering model or emitter.
 */
class NoriObject {
public:
    enum EClassType {
        EScene = 0,
        EMesh,
        EBSDF,
        EPhaseFunction,
        EEmitter,
        EMedium,
        ECamera,
        EIntegrator,
        ESampler,
        ETest,
        EReconstructionFilter,
        EClassTypeCount
    };

    /// Virtual destructor
    virtual ~NoriObject() { }

    /**
     * \brief Return the type of object (i.e. Mesh/BSDF/etc.) 
     * provided by this instance
     * */
    virtual EClassType getClassType() const = 0;

    /**
     * \brief Add a child object to the current instance
     *
     * The default implementation does not support children and
     * simply throws an exception
     */
    virtual void addChild(NoriObject *child);

    /**
     * \brief Set the parent object
     *
     * Subclasses may choose to override this method to be
     * notified when they are added to a parent object. The
     * default implementation does nothing.
     */
    virtual void setParent(NoriObject *parent);

    /**
     * \brief Perform some action associated with the object
     *
     * The default implementation throws an exception. Certain objects
     * may choose to override it, e.g. to implement initialization, 
     * testing, or rendering functionality.
     *
     * This function is called by the XML parser once it has
     * constructed an object and added all of its children
     * using \ref addChild().
     */
    virtual void activate();

    /// Return a brief string summary of the instance (for debugging purposes)
    virtual std::string toString() const = 0;
    
    /// Turn a class type into a human-readable string
    static std::string classTypeName(EClassType type) {
        switch (type) {
            case EScene:      return "scene";
            case EMesh:       return "mesh";
            case EBSDF:       return "bsdf";
            case EEmitter:    return "emitter";
            case ECamera:     return "camera";
            case EIntegrator: return "integrator";
            case ESampler:    return "sampler";
            case ETest:       return "test";
            default:          return "<unknown>";
        }
    }
};

/**
 * \brief Factory for Nori objects
 *
 * This utility class is part of a mini-RTTI framework and can 
 * instantiate arbitrary Nori objects by their name.
 */
class NoriObjectFactory {
public:
    typedef std::function<NoriObject *(const PropertyList &)> Constructor;

    /**
     * \brief Register an object constructor with the object factory
     *
     * This function is called by the macro \ref NORI_REGISTER_CLASS
     *
     * \param name
     *     An internal name that is associated with this class. This is the
     *     'type' field found in the scene description XML files
     *
     * \param constr
     *     A function pointer to an anonymous function that is
     *     able to call the constructor of the class.
     */
    static void registerClass(const std::string &name, const Constructor &constr);

    /**
     * \brief Construct an instance from the class of the given name
     *
     * \param name
     *     An internal name that is associated with this class. This is the
     *     'type' field found in the scene description XML files
     *
     * \param propList
     *     A list of properties that will be passed to the constructor
     *     of the class.
     */
    static NoriObject *createInstance(const std::string &name,
            const PropertyList &propList) {
        if (!m_constructors || m_constructors->find(name) == m_constructors->end())
            throw NoriException("A constructor for class \"%s\" could not be found!", name);
        return (*m_constructors)[name](propList);
    }
private:
    static std::map<std::string, Constructor> *m_constructors;
};

/// Macro for registering an object constructor with the \ref NoriObjectFactory
#define NORI_REGISTER_CLASS(cls, name) \
    cls *cls ##_create(const PropertyList &list) { \
        return new cls(list); \
    } \
    static struct cls ##_{ \
        cls ##_() { \
            NoriObjectFactory::registerClass(name, cls ##_create); \
        } \
    } cls ##__NORI_;

NORI_NAMESPACE_END
