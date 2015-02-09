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

#include <nori/parser.h>
#include <nori/proplist.h>
#include <Eigen/Geometry>
#include <pugixml.hpp>
#include <fstream>
#include <set>

NORI_NAMESPACE_BEGIN

NoriObject *loadFromXML(const std::string &filename) {
    /* Load the XML file using 'pugi' (a tiny self-contained XML parser implemented in C++) */
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());

    /* Helper function: map a position offset in bytes to a more readable line/column value */
    auto offset = [&](ptrdiff_t pos) -> std::string {
        std::fstream is(filename);
        char buffer[1024];
        int line = 0, linestart = 0, offset = 0;
        while (is.good()) {
            is.read(buffer, sizeof(buffer));
            for (int i = 0; i < is.gcount(); ++i) {
                if (buffer[i] == '\n') {
                    if (offset + i >= pos)
                        return tfm::format("line %i, col %i", line + 1, pos - linestart);
                    ++line;
                    linestart = offset + i;
                }
            }
            offset += (int) is.gcount();
        }
        return "byte offset " + std::to_string(pos);
    };

    if (!result) /* There was a parser / file IO error */
        throw NoriException("Error while parsing \"%s\": %s (at %s)", filename, result.description(), offset(result.offset));

    /* Set of supported XML tags */
    enum ETag {
        /* Object classes */
        EScene                = NoriObject::EScene,
        EMesh                 = NoriObject::EMesh,
        EBSDF                 = NoriObject::EBSDF,
        EPhaseFunction        = NoriObject::EPhaseFunction,
        EEmitter            = NoriObject::EEmitter,
        EMedium               = NoriObject::EMedium,
        ECamera               = NoriObject::ECamera,
        EIntegrator           = NoriObject::EIntegrator,
        ESampler              = NoriObject::ESampler,
        ETest                 = NoriObject::ETest,
        EReconstructionFilter = NoriObject::EReconstructionFilter,

        /* Properties */
        EBoolean = NoriObject::EClassTypeCount,
        EInteger,
        EFloat,
        EString,
        EPoint,
        EVector,
        EColor,
        ETransform,
        ETranslate,
        EMatrix,
        ERotate,
        EScale,
        ELookAt,

        EInvalid
    };

    /* Create a mapping from tag names to tag IDs */
    std::map<std::string, ETag> tags;
    tags["scene"]      = EScene;
    tags["mesh"]       = EMesh;
    tags["bsdf"]       = EBSDF;
    tags["emitter"]  = EEmitter;
    tags["camera"]     = ECamera;
    tags["medium"]     = EMedium;
    tags["phase"]      = EPhaseFunction;
    tags["integrator"] = EIntegrator;
    tags["sampler"]    = ESampler;
    tags["rfilter"]    = EReconstructionFilter;
    tags["test"]       = ETest;
    tags["boolean"]    = EBoolean;
    tags["integer"]    = EInteger;
    tags["float"]      = EFloat;
    tags["string"]     = EString;
    tags["point"]      = EPoint;
    tags["vector"]     = EVector;
    tags["color"]      = EColor;
    tags["transform"]  = ETransform;
    tags["translate"]  = ETranslate;
    tags["matrix"]     = EMatrix;
    tags["rotate"]     = ERotate;
    tags["scale"]      = EScale;
    tags["lookat"]     = ELookAt;

    /* Helper function to check if attributes are fully specified */
    auto check_attributes = [&](const pugi::xml_node &node, std::set<std::string> attrs) {
        for (auto attr : node.attributes()) {
            auto it = attrs.find(attr.name());
            if (it == attrs.end())
                throw NoriException("Error while parsing \"%s\": unexpected attribute \"%s\" in \"%s\" at %s",
                                    filename, attr.name(), node.name(), offset(node.offset_debug()));
            attrs.erase(it);
        }
        if (!attrs.empty())
            throw NoriException("Error while parsing \"%s\": missing attribute \"%s\" in \"%s\" at %s",
                                filename, *attrs.begin(), node.name(), offset(node.offset_debug()));
    };

    Eigen::Affine3f transform;

    /* Helper function to parse a Nori XML node (recursive) */
    std::function<NoriObject *(pugi::xml_node &, PropertyList &, int)> parseTag = [&](
        pugi::xml_node &node, PropertyList &list, int parentTag) -> NoriObject * {
        /* Skip over comments */
        if (node.type() == pugi::node_comment || node.type() == pugi::node_declaration)
            return nullptr;

        if (node.type() != pugi::node_element)
            throw NoriException(
                "Error while parsing \"%s\": unexpected content at %s",
                filename, offset(node.offset_debug()));

        /* Look up the name of the current element */
        auto it = tags.find(node.name());
        if (it == tags.end())
            throw NoriException("Error while parsing \"%s\": unexpected tag \"%s\" at %s",
                                filename, node.name(), offset(node.offset_debug()));
        int tag = it->second;

        /* Perform some safety checks to make sure that the XML tree really makes sense */
        bool hasParent            = parentTag != EInvalid;
        bool parentIsObject       = hasParent && parentTag < NoriObject::EClassTypeCount;
        bool currentIsObject      = tag < NoriObject::EClassTypeCount;
        bool parentIsTransform    = parentTag == ETransform;
        bool currentIsTransformOp = tag == ETranslate || tag == ERotate || tag == EScale || tag == ELookAt || tag == EMatrix;

        if (!hasParent && !currentIsObject)
            throw NoriException("Error while parsing \"%s\": root element \"%s\" must be a Nori object (at %s)",
                                filename, node.name(), offset(node.offset_debug()));

        if (parentIsTransform != currentIsTransformOp)
            throw NoriException("Error while parsing \"%s\": transform nodes "
                                "can only contain transform operations (at %s)",
                                filename,  offset(node.offset_debug()));

        if (hasParent && !parentIsObject && !(parentIsTransform && currentIsTransformOp))
            throw NoriException("Error while parsing \"%s\": node \"%s\" requires a Nori object as parent (at %s)",
                                filename, node.name(), offset(node.offset_debug()));

        if (tag == EScene)
            node.append_attribute("type") = "scene";
        else if (tag == ETransform)
            transform.setIdentity();

        PropertyList propList;
        std::vector<NoriObject *> children;
        for (pugi::xml_node &ch: node.children()) {
            NoriObject *child = parseTag(ch, propList, tag);
            if (child)
                children.push_back(child);
        }

        NoriObject *result = nullptr;
        try {
            if (currentIsObject) {
                check_attributes(node, { "type" });

                /* This is an object, first instantiate it */
                result = NoriObjectFactory::createInstance(
                    node.attribute("type").value(),
                    propList
                );

                if (result->getClassType() != (int) tag) {
                    throw NoriException(
                        "Unexpectedly constructed an object "
                        "of type <%s> (expected type <%s>): %s",
                        NoriObject::classTypeName(result->getClassType()),
                        NoriObject::classTypeName((NoriObject::EClassType) tag),
                        result->toString());
                }

                /* Add all children */
                for (auto ch: children) {
                    result->addChild(ch);
                    ch->setParent(result);
                }

                /* Activate / configure the object */
                result->activate();
            } else {
                /* This is a property */
                switch (tag) {
                    case EString: {
                            check_attributes(node, { "name", "value" });
                            list.setString(node.attribute("name").value(), node.attribute("value").value());
                        }
                        break;
                    case EFloat: {
                            check_attributes(node, { "name", "value" });
                            list.setFloat(node.attribute("name").value(), toFloat(node.attribute("value").value()));
                        }
                        break;
                    case EInteger: {
                            check_attributes(node, { "name", "value" });
                            list.setInteger(node.attribute("name").value(), toInt(node.attribute("value").value()));
                        }
                        break;
                    case EBoolean: {
                            check_attributes(node, { "name", "value" });
                            list.setBoolean(node.attribute("name").value(), toBool(node.attribute("value").value()));
                        }
                        break;
                    case EPoint: {
                            check_attributes(node, { "name", "value" });
                            list.setPoint(node.attribute("name").value(), Point3f(toVector3f(node.attribute("value").value())));
                        }
                        break;
                    case EVector: {
                            check_attributes(node, { "name", "value" });
                            list.setVector(node.attribute("name").value(), Vector3f(toVector3f(node.attribute("value").value())));
                        }
                        break;
                    case EColor: {
                            check_attributes(node, { "name", "value" });
                            list.setColor(node.attribute("name").value(), Color3f(toVector3f(node.attribute("value").value()).array()));
                        }
                        break;
                    case ETransform: {
                            check_attributes(node, { "name" });
                            list.setTransform(node.attribute("name").value(), transform.matrix());
                        }
                        break;
                    case ETranslate: {
                            check_attributes(node, { "value" });
                            Eigen::Vector3f v = toVector3f(node.attribute("value").value());
                            transform = Eigen::Translation<float, 3>(v.x(), v.y(), v.z()) * transform;
                        }
                        break;
                    case EMatrix: {
                            check_attributes(node, { "value" });
                            std::vector<std::string> tokens = tokenize(node.attribute("value").value());
                            if (tokens.size() != 16)
                                throw NoriException("Expected 16 values");
                            Eigen::Matrix4f matrix;
                            for (int i=0; i<4; ++i)
                                for (int j=0; j<4; ++j)
                                    matrix(i, j) = toFloat(tokens[i*4+j]);
                            transform = Eigen::Affine3f(matrix) * transform;
                        }
                        break;
                    case EScale: {
                            check_attributes(node, { "value" });
                            Eigen::Vector3f v = toVector3f(node.attribute("value").value());
                            transform = Eigen::DiagonalMatrix<float, 3>(v) * transform;
                        }
                        break;
                    case ERotate: {
                            check_attributes(node, { "angle", "axis" });
                            float angle = degToRad(toFloat(node.attribute("angle").value()));
                            Eigen::Vector3f axis = toVector3f(node.attribute("axis").value());
                            transform = Eigen::AngleAxis<float>(angle, axis) * transform;
                        }
                        break;
                    case ELookAt: {
                            check_attributes(node, { "origin", "target", "up" });
                            Eigen::Vector3f origin = toVector3f(node.attribute("origin").value());
                            Eigen::Vector3f target = toVector3f(node.attribute("target").value());
                            Eigen::Vector3f up = toVector3f(node.attribute("up").value());

                            Vector3f dir = (target - origin).normalized();
                            Vector3f left = up.normalized().cross(dir).normalized();
                            Vector3f newUp = dir.cross(left).normalized();

                            Eigen::Matrix4f trafo;
                            trafo << left, newUp, dir, origin,
                                      0, 0, 0, 1;

                            transform = Eigen::Affine3f(trafo) * transform;
                        }
                        break;

                    default: throw NoriException("Unhandled element \"%s\"", node.name());
                };
            }
        } catch (const NoriException &e) {
            throw NoriException("Error while parsing \"%s\": %s (at %s)", filename,
                                e.what(), offset(node.offset_debug()));
        }

        return result;
    };

    PropertyList list;
    return parseTag(*doc.begin(), list, EInvalid);
}

NORI_NAMESPACE_END
