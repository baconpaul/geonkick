/**
 * File name: mainwindow.cpp
 * Project: Geonkick (A kick synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor (http://geontime.com)
 *
 * This file is part of Geonkick.
 *
 * GeonKick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "mainwindow.h"
#include "oscillator.h"
#include "envelope_widget.h"
#include "oscillator_group_box.h"
#include "general_group_box.h"
#include "control_area.h"
#include "top_bar.h"
#include "limiter.h"
#include "export_widget.h"
#include "geonkick_api.h"
#include "geonkick_state.h"
#include "about.h"

#include <RkEvent.h>

MainWindow::MainWindow(RkMain *app, GeonkickApi *api)
        : GeonkickWidget(app)
        , geonkickApi{api}
        , topBar{nullptr}
        , envelopeWidget{nullptr}
        //        presetName(preset)
{
        setFixedSize(940, 760);
        setTitle(GEOKICK_APP_NAME);
        geonkickApi->registerCallbacks(true);
//        setWindowIcon(QPixmap(":/app_icon.png"));
        show();
}

MainWindow::MainWindow(RkMain *app, GeonkickApi *api, const RkNativeWindowInfo &info)
        : GeonkickWidget(app, info)
        , geonkickApi{api}
        , topBar{nullptr}
        , envelopeWidget{nullptr}
          //        presetName(preset)
{
        setFixedSize(940, 760);
        setTitle(GEOKICK_APP_NAME);
        geonkickApi->registerCallbacks(true);
//        setWindowIcon(QPixmap(":/app_icon.png"));
        show();
}

MainWindow::~MainWindow()
{
        if (geonkickApi) {
                geonkickApi->registerCallbacks(false);
                geonkickApi->setEventQueue(nullptr);
                // Since for plugins the api is not destroyed there
                // is a need to unbind from the GUI that is being detryied.
                RK_ACT_UNBIND_ALL(geonkickApi, kickLengthUpdated);
                RK_ACT_UNBIND_ALL(geonkickApi, kickAmplitudeUpdated);
                RK_ACT_UNBIND_ALL(geonkickApi, kickUpdated);
                RK_ACT_UNBIND_ALL(geonkickApi, newKickBuffer);
                RK_ACT_UNBIND_ALL(geonkickApi, currentPlayingFrameVal);
                if (geonkickApi->isStandalone())
                        delete geonkickApi;
        }
}

bool MainWindow::init(void)
{
        oscillators = geonkickApi->oscillators();
        /*        if (geonkickApi->isStandalone() && !geonkickApi->isJackEnabled())
                QMessageBox::warning(this, "Warning - Geonkick", tr("Jack is not installed" \
                                     " or not running. There is a need for jack server running " \
                                     "in order to have audio output."),
                                     QMessageBox::Ok);
        */
        auto topBar = new TopBar(this, geonkickApi);
        topBar->setX(10);
        topBar->show();
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), topBar, updateGui());
        RK_ACT_BIND(topBar, openFile, RK_ACT_ARGS(), this, openFileDialog(FileDialog::Type::Open));
        RK_ACT_BIND(topBar, saveFile, RK_ACT_ARGS(), this, openFileDialog(FileDialog::Type::Save));
        RK_ACT_BIND(topBar, openAbout, RK_ACT_ARGS(), this, openAboutDialog());
        RK_ACT_BIND(topBar, openExport, RK_ACT_ARGS(), this, openExportDialog());
        RK_ACT_BIND(topBar, layerSelected, RK_ACT_ARGS(GeonkickApi::Layer layer, bool b), geonkickApi, enbaleLayer(layer, b));
        // Create envelope widget.
        envelopeWidget = new EnvelopeWidget(this, geonkickApi, oscillators);
        envelopeWidget->setX(10);
        envelopeWidget->setY(topBar->y() + topBar->height());
        envelopeWidget->setFixedSize(850, 340);
        envelopeWidget->show();
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), envelopeWidget, updateGui());
        auto limiterWidget = new Limiter(geonkickApi, this);
        limiterWidget->setPosition(envelopeWidget->x() + envelopeWidget->width() + 8, envelopeWidget->y());
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), limiterWidget, onUpdateLimiter());
        limiterWidget->show();
        controlAreaWidget = new ControlArea(this, geonkickApi, oscillators);
        controlAreaWidget->setPosition(10, envelopeWidget->y() + envelopeWidget->height() + 3);
        RK_ACT_BIND(this, updateGui, RK_ACT_ARGS(), controlAreaWidget, updateGui());
        controlAreaWidget->show();
        //        if (!presetName.isEmpty()) {
        //                setPreset(presetName);
        //                updateGui();
        //         }
        return true;
}

void MainWindow::openExportDialog()
{
        new ExportWidget(this, geonkickApi);
}

void MainWindow::savePreset(const std::string &fileName)
{
        if (fileName.size() < 6) {
                RK_LOG_ERROR("Save Preset: " << "Can't save preset. File name empty or wrong format. Format example: 'mykick.gkick'");
                return;
        }

        std::filesystem::path filePath(fileName);
        std::locale loc;
        if (filePath.extension().empty()
            || (filePath.extension() != ".gkick"
            && filePath.extension() != ".GKICK"))
                filePath.replace_extension(".gkick");

        std::ofstream file;
        file.open(std::filesystem::absolute(filePath));
        if (!file.is_open()) {
                RK_LOG_ERROR("Error | Save Preset" + std::string(" - ") + std::string(GEOKICK_APP_NAME) << ". Can't save preset");
                return;
        }
        file << geonkickApi->getState()->toJson();
        file.close();
        topBar->setPresetName(filePath.filename());
}

void MainWindow::openPreset(const std::string &fileName)
{
        if (fileName.size() < 6) {
                RK_LOG_ERROR("Open Preset: " << "Can't save preset. File name empty or wrong format. Format example: 'mykick.gkick'");
                return;
        }

        std::filesystem::path filePath(fileName);
        if (filePath.extension().empty()
            || (filePath.extension() != ".gkick"
            && filePath.extension() != ".GKICK")) {
                RK_LOG_ERROR("Open Preset: " << "Can't open preset. Wrong file format.");
                return;
        }

        std::ifstream file;
        file.open(std::filesystem::absolute(filePath));
        if (!file.is_open()) {
                RK_LOG_ERROR("Open Preset" + std::string(" - ") + std::string(GEOKICK_APP_NAME) << ". Can't open preset.");
                return;
        }

        std::string fileData((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
        auto state = std::make_shared<GeonkickState>(fileData);
        geonkickApi->setState(state);
        //                topBar->setPresetName(QFileInfo(file).baseName());
        file.close();
        updateGui();
}

void MainWindow::openFileDialog(FileDialog::Type type)
{
        auto fileDialog = new FileDialog(this, type, type == FileDialog::Type::Open ? "Open Preset" : "Save Preset");
        fileDialog->exec();
        if (type == FileDialog::Type::Open)
                openPreset(fileDialog->filePath());
        else
                savePreset(fileDialog->filePath());
}

void MainWindow::openAboutDialog()
{
        new AboutDialog(this);
}

void MainWindow::keyPressEvent(const std::shared_ptr<RkKeyEvent> &event)
{
        if (event->key() == Rk::Key::Key_k) {
                geonkickApi->setKeyPressed(true, 127);
        } else if (event->modifiers() ==  static_cast<int>(Rk::KeyModifiers::Control)
                   && event->key() == Rk::Key::Key_r) {
                geonkickApi->setState(geonkickApi->getDefaultState());
                topBar->setPresetName("");
                updateGui();
        } else if (event->modifiers() == static_cast<int>(Rk::KeyModifiers::Control)
                   && event->key() == Rk::Key::Key_h) {
                envelopeWidget->hideEnvelope(true);
        }
}

void MainWindow::keyReleaseEvent(const std::shared_ptr<RkKeyEvent> &event)
{
        if (event->modifiers() ==  static_cast<int>(Rk::KeyModifiers::Control)
            && event->key() == Rk::Key::Key_h) {
                envelopeWidget->hideEnvelope(false);
        }
}

