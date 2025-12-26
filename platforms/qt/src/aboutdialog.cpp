#include "aboutdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About CutieCoCo"));
    setFixedSize(400, 320);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // Application name and version
    auto* titleLabel = new QLabel(
        QString("<h2>CutieCoCo</h2>"
                "<p>Version %1</p>")
            .arg(QApplication::applicationVersion()),
        this);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // Description
    auto* descLabel = new QLabel(
        tr("<p>A cross-platform Tandy Color Computer 3 (CoCo 3) emulator.</p>"
           "<p>Supports Motorola 6809 and Hitachi 6309 CPUs with memory "
           "expansion up to 8192K.</p>"),
        this);
    descLabel->setWordWrap(true);
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);

    // Credits/lineage
    auto* creditsLabel = new QLabel(
        tr("<p><b>Lineage:</b><br>"
           "VCC &rarr; VCCE &rarr; DREAM-VCC &rarr; CutieCoCo</p>"
           "<p><b>Original VCC by:</b> Joseph Forgione<br>"
           "<b>Contributors:</b> The VCC community</p>"),
        this);
    creditsLabel->setWordWrap(true);
    creditsLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(creditsLabel);

    // License
    auto* licenseLabel = new QLabel(
        tr("<p><small>This program is free software under the GNU General Public License v3.</small></p>"),
        this);
    licenseLabel->setWordWrap(true);
    licenseLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(licenseLabel);

    layout->addStretch();

    // OK button
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto* okButton = new QPushButton(tr("OK"), this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
}
