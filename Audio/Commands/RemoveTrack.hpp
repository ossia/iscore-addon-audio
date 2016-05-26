#pragma once
#include <Audio/Commands/AudioCommandFactory.hpp>
#include <iscore/command/SerializableCommand.hpp>
#include <Audio/Panel/Track.hpp>
#include <iscore/tools/TreePath.hpp>

namespace Audio {
namespace Panel {
    class TrackModel;
}

namespace Commands {

class RemoveTrack : public iscore::SerializableCommand {
    ISCORE_COMMAND_DECL(CommandFactoryName(), RemoveTrack, "Remove this track")
public:
    RemoveTrack(Path<Panel::TrackModel> device_tree, int index);
    void undo() const override;
    void redo() const override;
    int m_index;

protected:
    void serializeImpl(DataStreamInput&) const override;
    void deserializeImpl(DataStreamOutput&) override;

private:
    Path<Panel::TrackModel> m_devicesModel;
    Panel::Track m_track;
};

}
}
