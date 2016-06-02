#include "MixInspector.hpp"
#include <Audio/Commands/UpdateMix.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <iscore/widgets/SignalUtils.hpp>

#include <QCheckBox>
#include <QSpinBox>
#include <QTableWidget>

namespace Audio
{
namespace Mix
{
InspectorWidget::InspectorWidget(
        const Mix::ProcessModel &object,
        const iscore::DocumentContext &doc,
        QWidget *parent):
    InspectorWidgetDelegate_T {object, parent},
    m_dispatcher{doc.commandStack}
{
    setObjectName("MixInspectorWidget");

    auto lay = new QFormLayout;
    this->setLayout(lay);

    m_table = new QTableWidget{this};
    recreate();

    con(object, &Mix::ProcessModel::routingChanged,
        this, &InspectorWidget::recreate);
}

class MixSpinBox : public QSpinBox
{
    public:
        MixSpinBox(QWidget* parent):
            QSpinBox{parent}
        {
            setMinimum(0);
            setMaximum(100);
            setSizePolicy(QSizePolicy::Minimum, {});
            setAlignment(Qt::AlignLeft);
        }
};

class RoutingTableWidget : public QWidget
{
    public:
        RoutingTableWidget(
                QWidget* parent,
                OngoingCommandDispatcher& dispatcher,
                const Mix::ProcessModel& mix,
                const Id<Process::ProcessModel>& column,
                const Id<Process::ProcessModel>& row):
            QWidget{parent},
            checkBox{new QCheckBox{this}},
            spinBox{new MixSpinBox{this}}
        {
            auto lay = new QVBoxLayout;
            this->setLayout(lay);

            lay->addWidget(spinBox);
            lay->addWidget(checkBox);

            spinBox->setValue(mix.mix(column, row) * 100);
            checkBox->setChecked(mix.routings().find(Routing{column, row})->enabled);
            connect(spinBox, SignalUtils::QSpinBox_valueChanged_int(),
                    this, [=,&mix,&dispatcher] (int val) {
                dispatcher.submitCommand<Audio::Commands::UpdateRouting>(
                                mix,
                                Routing{column, row, val / 100., bool(checkBox->checkState())});
            });

            connect(spinBox, &QSpinBox::editingFinished,
                    this, [&dispatcher] () {
                dispatcher.commit();
            });

            connect(checkBox, &QCheckBox::stateChanged,
                    this, [=,&mix,&dispatcher] (int check) {
                // TODO check if different
                dispatcher.submitCommand<Audio::Commands::UpdateRouting>(
                                mix,
                                Routing{column, row, spinBox->value() / 100., bool(check)});
                dispatcher.commit();

            } );
        }

    private:
        QCheckBox* const checkBox{};
        MixSpinBox* const spinBox{};

};
void InspectorWidget::recreate()
{
    m_table->clear();
    // Row : input processes & direct outputs (fx, send, direct)
    // Column : output processes (sound, scenario, fx, etc.)
    const Mix::ProcessModel& mix = process();
    int n_data = mix.directData().size();
    int n_fx = mix.directFx().size();
    int n_sends = mix.sends().size();

    int n_row = n_fx + n_sends + 1;
    int n_col = n_data + n_fx;

    m_table->setRowCount(n_row);
    m_table->setColumnCount(n_col);

    auto cst = safe_cast<const Scenario::ConstraintModel*>(process().parent());

    // For each direct data, create relevant items.
    auto pretty_name = [&] (const Id<Process::ProcessModel>& dmx) {

        return cst->processes.at(dmx).objectName()
                + " ("
                + QString::number(*dmx.val())
                + ")";
    };

    QStringList col_labels;
    QStringList row_labels;
    auto col_it = mix.directData().begin();
    for(int col = 0; col < n_data; col++)
    {
        col_labels.push_back(pretty_name(col_it->process));

        // Data -> Fx
        auto fx_it = mix.directFx().begin();
        for(int row = 0; row < n_fx; row++)
        {
            auto sb = new RoutingTableWidget{m_table, m_dispatcher, mix, col_it->process, fx_it->process};
            m_table->setCellWidget(row, col, sb);

            row_labels.push_back(pretty_name(fx_it->process));
            fx_it++;
        }

        // Data -> Send
        auto send_it = mix.sends().begin();
        for(int row = n_fx; row < n_fx + n_sends; row++)
        {
            auto sb = new RoutingTableWidget{m_table, m_dispatcher, mix, col_it->process, *send_it};
            m_table->setCellWidget(row, col, sb);

            row_labels.push_back(pretty_name(*send_it));
            send_it++;
        }

        // Data -> Direct
        auto sb = new MixSpinBox{m_table};
        sb->setValue(mix.mix(col_it->process));
        m_table->setCellWidget(n_fx + n_sends, col, sb);

        connect(sb, SignalUtils::QSpinBox_valueChanged_int(),
                this, [=,&mix] (int val) {
            m_dispatcher.submitCommand<Audio::Commands::UpdateDirect>(
                            mix,
                            DirectMix{col_it->process, val / 100.});
        });
        connect(sb, &QSpinBox::editingFinished,
                this, [=] () { m_dispatcher.commit(); });


        row_labels.push_back(tr("Direct"));
        col_it++;
    }

    // For each fx, create relevant items.
    col_it = mix.directFx().begin();
    for(int col = n_data; col < (n_data + n_fx); col++)
    {
        col_labels.push_back(pretty_name(col_it->process));

        // Fx -> Send
        auto send_it = mix.sends().begin();
        for(int row = n_fx; row < n_fx + n_sends; row++)
        {
            auto sb = new RoutingTableWidget{m_table, m_dispatcher, mix, col_it->process, *send_it};
            m_table->setCellWidget(row, col, sb);

            send_it++;
        }

        // Fx -> Direct
        auto sb = new MixSpinBox{m_table};
        sb->setValue(mix.mix(col_it->process));
        m_table->setCellWidget(n_fx + n_sends, col, sb);

        connect(sb, SignalUtils::QSpinBox_valueChanged_int(),
                this, [=,&mix] (int val) {
            m_dispatcher.submitCommand<Audio::Commands::UpdateDirect>(
                            mix,
                            DirectMix{col_it->process, val / 100.});
        });
        connect(sb, &QSpinBox::editingFinished,
                this, [=] () { m_dispatcher.commit(); });

        col_it++;
    }
    m_table->setHorizontalHeaderLabels(col_labels);
    m_table->setVerticalHeaderLabels(row_labels);

    this->layout()->addWidget(m_table);
}

}
}