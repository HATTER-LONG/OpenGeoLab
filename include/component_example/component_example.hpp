#pragma once

#include <iostream>
#include <kangaroo/util/component_factory.hpp>
#include <ostream>

namespace Example {
/**
 * @brief Abstract interface class for factory-created objects
 *
 * This interface defines the contract that all objects created by factories
 * must implement. It provides methods for identification and data access.
 */
class InterfaceClass {
public:
    InterfaceClass() = default;
    virtual ~InterfaceClass() = default;

    /**
     * @brief Get the unique product identifier for this class type
     * @return std::string Unique identifier string
     */
    virtual std::string getClassProductId() = 0;

    /**
     * @brief Get the data stored in this object instance
     * @return std::string Object's data content
     */
    virtual std::string getData() = 0;
};

/**
 * @brief Concrete implementation of InterfaceClass for testing
 *
 * This class provides a basic implementation of the interface with
 * configurable data storage and a static product identifier.
 */
class DerivedClass : public InterfaceClass {
public:
    DerivedClass() = default;
    ~DerivedClass() override = default;
    explicit DerivedClass(std::string str) : m_data(str) {}

    std::string getClassProductId() override { return productId(); };
    std::string getData() override { return m_data; }

    /** @brief Static product identifier for this class type */
    static std::string productId() { return "DerivedClassProduct"; }

    std::string m_data{""};
};

// =============================================================================
// Factory Interface and Implementation Classes
// =============================================================================

/**
 * @brief Factory interface for creating InterfaceClass objects
 *
 * This factory defines the standard creation methods that implementations
 * must provide. It supports both parameterless and parameterized object creation,
 * as well as dependency injection through ComponentFactoryInjector.
 */
class IobjFactory : public Kangaroo::Util::FactoryTraits<IobjFactory, InterfaceClass> {
public:
    virtual ~IobjFactory() = default;

    /** @brief Create object without parameters */
    virtual tObjectPtr create() const = 0;

    /** @brief Create object with string parameter */
    virtual tObjectPtr create(const std::string& str) const = 0;

    /** @brief Create object with dependency injection support */
    virtual tObjectPtr create(const std::string&, Kangaroo::Util::ComponentFactoryInjector&) const {
        return nullptr;
    };
};

/**
 * @brief Standard factory implementation for DerivedClass
 *
 * This factory provides concrete implementation for creating DerivedClass
 * instances with various construction parameters.
 */
class NewDerivedclassFactory : public IobjFactory {
public:
    virtual tObjectPtr create() const override { return std::make_unique<DerivedClass>(); }

    virtual tObjectPtr create(const std::string& str) const override {
        return std::make_unique<DerivedClass>(str);
    }
};

inline void exampleUsage() {
    // Register the factory with the component factory system
    g_ComponentFactory.registFactoryWithID<NewDerivedclassFactory>("DerivedClassProduct");

    // Create an object using the factory
    auto obj = g_ComponentFactory.createObjectWithID<IobjFactory>("DerivedClassProduct",
                                                                  "Hello, Kangaroo!");

    // Use the created object
    if(obj) {
        std::cout << "Product ID: " << obj->getClassProductId() << std::endl;
        std::cout << "Data: " << obj->getData() << std::endl;
    } else {
        std::cout << "Failed to create object." << std::endl;
    }
}
} // namespace Example