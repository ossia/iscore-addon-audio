#include "EffectSlider.hpp"
#include <ossia/network/domain/domain.hpp>
#include <State/Expression.hpp>
#include <iscore/widgets/DoubleSlider.hpp>

#include <ossia/network/base/node.hpp>
#include <ossia/editor/value/value_conversion.hpp>
#include <QMenu>
#include <QLabel>
#include <QVBoxLayout>
#include <QContextMenuEvent>

namespace Audio
{
namespace Effect
{

EffectSlider::EffectSlider(const ossia::net::node_base& fx, QWidget* parent):
  QWidget{parent},
  m_param{fx}
{
  auto addr = m_param.getAddress();
  auto dom = addr->getDomain();

  if(auto f = ossia::net::min(dom).try_get<ossia::Float>()) m_min = f->value;
  if(auto f = ossia::net::max(dom).try_get<ossia::Float>()) m_max = f->value;

  auto lay = new QVBoxLayout;
  lay->addWidget(new QLabel{QString::fromStdString(m_param.getName())});
  m_slider = new iscore::DoubleSlider{this};
  lay->addWidget(m_slider);
  this->setLayout(lay);

  auto cur_val = ossia::convert<float>(m_param.getAddress()->cloneValue());
  m_slider->setValue((cur_val - m_min) / (m_max - m_min));

  connect(m_slider, &iscore::DoubleSlider::valueChanged,
          this, [=] (double v)
  {
    // v is between 0 - 1
    auto cur = m_param.getAddress()->cloneValue();
    auto exp = float(m_min + (m_max - m_min) * v);
    if(ossia::convert<float>(cur) != exp)
      m_param.getAddress()->pushValue(ossia::Float{exp});

  });
/*
  m_callback = addr->add_callback([=] (const ossia::value& val)
  {
    if(auto v = val.try_get<ossia::Float>())
    {
      auto scaled = (v - m_min) / (m_max - m_min)
      if(scaled != m_slider->value()) // TODO qFuzzyCompare instead
        m_slider->setValue(scaled);
    }
  });
*/
  m_addAutomAction = new QAction{tr("Add automation")};
  connect(m_addAutomAction, &QAction::triggered,
          this, [=] () {
    auto& ossia_addr = *m_param.getAddress();
    auto& dom = ossia_addr.getDomain();
    auto addr = State::parseAddress(QString::fromStdString(ossia::net::address_string_from_node(ossia_addr)));

    if(addr)
    {
      auto min = ossia::convert<double>(ossia::net::min(dom));
      auto max = ossia::convert<double>(ossia::net::max(dom));
      emit createAutomation(std::move(*addr), min, max);
    }
  });
  // TODO show tooltip with current value
}

EffectSlider::~EffectSlider()
{/*
  if(auto addr = m_param.getAddress())
  {
    addr->remove_callback(m_callback);
  }
  */
}

void EffectSlider::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu* contextMenu = new QMenu{this};

  contextMenu->addAction(m_addAutomAction);
  contextMenu->exec(event->globalPos());

  contextMenu->deleteLater();
}

}
}
