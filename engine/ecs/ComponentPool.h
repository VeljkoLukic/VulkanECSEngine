#pragma once
#include "engine/ecs/Types.h"
#include <vector>
#include <cassert>

namespace ecs {

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(EntityID entity) = 0;
    virtual bool has(EntityID entity) const noexcept = 0;
};

template<typename T>
class ComponentPool final : public IComponentPool {
public:
    T& add(EntityID entity, T component) {
        assert(!has(entity) && "Entity already owns this component type");

        if (entity >= m_sparse.size())
            m_sparse.resize(static_cast<std::size_t>(entity) + 1, NullEntity);

        m_sparse[entity] = static_cast<EntityID>(m_dense.size());
        m_dense.push_back(entity);
        return m_components.emplace_back(std::move(component));
    }

    void remove(EntityID entity) override {
        assert(has(entity) && "Entity does not own this component type");

        const EntityID denseIdx   = m_sparse[entity];
        const EntityID lastEntity = m_dense.back();

        m_components[denseIdx] = std::move(m_components.back());
        m_dense[denseIdx]      = lastEntity;
        m_sparse[lastEntity]   = denseIdx;
        m_sparse[entity]       = NullEntity;

        m_dense.pop_back();
        m_components.pop_back();
    }

    [[nodiscard]] bool has(EntityID entity) const noexcept override {
        return entity < m_sparse.size() && m_sparse[entity] != NullEntity;
    }

    [[nodiscard]] T&       get(EntityID entity)       noexcept { assert(has(entity)); return m_components[m_sparse[entity]]; }
    [[nodiscard]] const T& get(EntityID entity) const noexcept { assert(has(entity)); return m_components[m_sparse[entity]]; }

    [[nodiscard]] std::vector<T>&        components() noexcept { return m_components; }
    [[nodiscard]] std::vector<EntityID>& entities()   noexcept { return m_dense; }
    [[nodiscard]] std::size_t            size() const noexcept { return m_dense.size(); }

private:
    std::vector<EntityID> m_sparse;
    std::vector<EntityID> m_dense;
    std::vector<T>        m_components;
};

}
