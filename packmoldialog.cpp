/**********************************************************************
  PackmolDialog - Dialog for generating cubes and meshes

  Copyright (C) 2010 Tim Vandermeersch

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  Avogadro is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Avogadro is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include "packmoldialog.h"
#include "highlighter.h"

#include <Eigen/Core>

#include <avogadro/atom.h>
#include <avogadro/molecule.h>
#include <avogadro/moleculefile.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDebug>

namespace Avogadro {

  struct Volume
  {
    enum Type { Cube, Box, Sphere, Ellipsoid, Cylinder, Plane, NoType };
    enum Constraint { Inside, Outside, Above, Below, NoConstraint };

    Volume() : type(NoType), constraint(NoConstraint) {}
    Volume(Type _type, Constraint _constraint) : type(_type), constraint(_constraint) {}

    Type type;
    Constraint constraint;
  };

  struct Structure
  {
    Structure(QString _fileName) : fileName(_fileName) {}

    QString fileName;
    int number;
    Volume volume;
  };

  PackmolDialog::PackmolDialog(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f), m_process(0)
  {
    ui.setupUi(this);
    new Highlighter(ui.textEdit->document());

    // Connect up some signals and slots
    connect(ui.solvSoluteBrowse, SIGNAL(clicked()), this, SLOT(solvSoluteBrowseClicked()));
    connect(ui.solvSolventBrowse, SIGNAL(clicked()), this, SLOT(solvSolventBrowseClicked()));
    connect(ui.solvGenerate, SIGNAL(clicked()), this, SLOT(solvGenerateClicked()));
    connect(ui.solvAdjustShape, SIGNAL(stateChanged(int)), this, SLOT(solvAdjustShapeClicked(int)));
    connect(ui.solvAddCounterIons, SIGNAL(stateChanged(int)), this, SLOT(solvAddCounterIonsClicked(int)));
    connect(ui.solvGuessSolventNumber, SIGNAL(stateChanged(int)), this, SLOT(solvGuessSolventNumberClicked(int)));
    
    connect(ui.runButton, SIGNAL(clicked()), this, SLOT(runButtonClicked()));
  }

  PackmolDialog::~PackmolDialog()
  {
  }

  void PackmolDialog::solvSoluteBrowseClicked()
  {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Molecule"));
    ui.solvSoluteFilename->setText(fileName);
    solvUpdateVolume();
  }
 
  void PackmolDialog::solvSolventBrowseClicked()
  {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Molecule"));
    ui.solvSolventFilename->setText(fileName);
  }
  
  void PackmolDialog::solvAdjustShapeClicked(int state)
  {
    switch (state) {
      default:
      case Qt::Unchecked:
        ui.solvMinX->setEnabled(true);
        ui.solvMinY->setEnabled(true);
        ui.solvMinZ->setEnabled(true);
        ui.solvMaxX->setEnabled(true);
        ui.solvMaxY->setEnabled(true);
        ui.solvMaxZ->setEnabled(true);
        ui.solvCenterX->setEnabled(true);
        ui.solvCenterY->setEnabled(true);
        ui.solvCenterZ->setEnabled(true);
        ui.solvRadius->setEnabled(true);
        ui.solvSpacing->setEnabled(false);
        break;
      case Qt::Checked:
        ui.solvMinX->setEnabled(false);
        ui.solvMinY->setEnabled(false);
        ui.solvMinZ->setEnabled(false);
        ui.solvMaxX->setEnabled(false);
        ui.solvMaxY->setEnabled(false);
        ui.solvMaxZ->setEnabled(false);
        ui.solvCenterX->setEnabled(false);
        ui.solvCenterY->setEnabled(false);
        ui.solvCenterZ->setEnabled(false);
        ui.solvRadius->setEnabled(false);
        ui.solvSpacing->setEnabled(true);
        break;
    }

    solvUpdateVolume();
  }
  
  void PackmolDialog::solvUpdateVolume()
  {
    if (!ui.solvAdjustShape->isChecked())
      return;

    QString soluteFilename = ui.solvSoluteFilename->text();
    if (soluteFilename.length()) {
      Molecule *molecule = MoleculeFile::readMolecule(soluteFilename);
      if (molecule) {
        double spacing = ui.solvSpacing->value();
        
        // Box
        double minX, minY, minZ;
        double maxX, maxY, maxZ;
        minX = minY = minZ = 1000.0;
        maxX = maxY = maxZ = -1000.0;

        Eigen::Vector3d center(Eigen::Vector3d::Zero());
        foreach(Atom *atom, molecule->atoms()) {
          Eigen::Vector3d pos = *(atom->pos());
          center += pos;
          if (pos.x() < minX) minX = pos.x();
          if (pos.y() < minY) minY = pos.y();
          if (pos.z() < minZ) minZ = pos.z();
          if (pos.x() > maxX) maxX = pos.x();
          if (pos.y() > maxY) maxY = pos.y();
          if (pos.z() > maxZ) maxZ = pos.z();
        }       
        center /= molecule->numAtoms();
    
        ui.solvMinX->setValue(minX - spacing);
        ui.solvMinY->setValue(minY - spacing);
        ui.solvMinZ->setValue(minZ - spacing);
        ui.solvMaxX->setValue(maxX + spacing);
        ui.solvMaxY->setValue(maxY + spacing);
        ui.solvMaxZ->setValue(maxZ + spacing);
          
        // Sphere        
        double maxR = 0.0;
        foreach(Atom *atom, molecule->atoms()) {
          double r = (center - *(atom->pos())).norm();
          if (r > maxR) 
            maxR = r;
        }

        ui.solvCenterX->setValue(center.x());
        ui.solvCenterY->setValue(center.y());
        ui.solvCenterZ->setValue(center.z());
        ui.solvRadius->setValue(maxR + spacing);
      }
    }
  }
    
  double PackmolDialog::solvCalcVolume()
  {
    if (ui.solvShape->currentIndex() == 0) {
      // Box
      double dx = ui.solvMaxX->value() - ui.solvMinX->value();
      double dy = ui.solvMaxY->value() - ui.solvMinY->value();
      double dz = ui.solvMaxZ->value() - ui.solvMinZ->value();
      return dx * dy * dz;
    } else {
      // Sphere
      double r = ui.solvRadius->value();
      return 4.0 / 3.0 * M_PI * r * r * r;
    }
  }
    
  void PackmolDialog::solvUpdateSoluteNumber()
  {
    if (!ui.solvGuessSolventNumber->isChecked())
      return;

    double volume = solvCalcVolume();
    int number = 0.09 * volume + 19.75; // correlation

    ui.solvSolventNumber->setValue(number);  
  }

  void PackmolDialog::solvAddCounterIonsClicked(int state)
  {
  }
  
  void PackmolDialog::solvGuessSolventNumberClicked(int state)
  {
    switch (state) {
      default:
      case Qt::Unchecked:
        ui.solvSolventNumber->setEnabled(true);
        break;
      case Qt::Checked:
        ui.solvSolventNumber->setEnabled(false);
        break;
    }
  
    solvUpdateSoluteNumber();
  }


  void PackmolDialog::solvGenerateClicked()
  {
    if (ui.solvSolventFilename->text().length() == 0) {
      QMessageBox::question(this, tr("No solvent"), tr("No solvent filename specified."));
      return;
    }
    if (ui.solvSoluteFilename->text().length() == 0) {
      QMessageBox::StandardButton result = QMessageBox::question(this, tr("No solute"), 
          tr("No solute filename specified. Continue?"), QMessageBox::Yes | QMessageBox::No);
      if (result == QMessageBox::No)
        return;
    }

    ui.tabWidget->setCurrentIndex(1); // change to text mode

    QString filetype = ui.filetype->currentText();

    QString text;
    text += "tolerance " + QString::number(ui.tolerance->value(), 'f', 1) + "\n";
    text += "filetype " +  filetype + "\n";
    text += "output " + ui.output->text() + "\n";
    text += "\n";
    // solute
    if (ui.solvSoluteFilename->text().length() > 0) {
      QFileInfo soluteFileInfo(ui.solvSoluteFilename->text());
      text += "structure " + soluteFileInfo.baseName() + "." + filetype + "\n";
      text += "  number " + QString::number(ui.solvSoluteNumber->value()) + "\n";
      text += "end structure\n";
      text += "\n";
    }
    // solvent
    QFileInfo solventFileInfo(ui.solvSolventFilename->text());
    text += "structure " + solventFileInfo.baseName() + "." + filetype + "\n";
    text += "  number " + QString::number(ui.solvSolventNumber->value()) + "\n";
    if (ui.solvShape->currentIndex() == 0) {
      // Box
      text += "  inside box " + QString::number(ui.solvMinX->value(), 'f', 1) + " "
                              + QString::number(ui.solvMinY->value(), 'f', 1) + " "
                              + QString::number(ui.solvMinZ->value(), 'f', 1) + " "
                              + QString::number(ui.solvMaxX->value(), 'f', 1) + " "
                              + QString::number(ui.solvMaxY->value(), 'f', 1) + " "
                              + QString::number(ui.solvMaxZ->value(), 'f', 1) + "\n";
    } else {
      // Sphere
      text += "  inside sphere " + QString::number(ui.solvCenterX->value(), 'f', 1) + " "
                                 + QString::number(ui.solvCenterY->value(), 'f', 1) + " "
                                 + QString::number(ui.solvCenterZ->value(), 'f', 1) + " "
                                 + QString::number(ui.solvRadius->value(), 'f', 1) + "\n";
    }
    text += "end structure\n";
    text += "\n";
 
    ui.textEdit->setText(text);
  }
    
  void PackmolDialog::runButtonClicked()
  {
    if (m_process) {
      m_process->deleteLater();
      ui.tabWidget->setCurrentIndex(2); // change to output mode
      return;
    }
    QString program = "/usr/local/bin/packmol"; // FIXME: should be option
    
    ui.runButton->setEnabled(false);
    ui.abortButton->setEnabled(true);

    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), 
        this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(updateStandardOutput()));

    QString tmpdir = QDesktopServices::storageLocation(QDesktopServices::TempLocation);

    QFile inputFile(tmpdir + "/input.inp");
    if (!inputFile.open(QIODevice::WriteOnly | QIODevice::Text))
      return;

    QTextStream stream(&inputFile);
    stream << ui.textEdit->toPlainText().toAscii();
    inputFile.close();

    m_process->setStandardInputFile(tmpdir + "/input.inp");
    m_process->start(program);

    ui.tabWidget->setCurrentIndex(2); // change to output mode
  }
  
  void PackmolDialog::updateStandardOutput()
  {
    ui.outputEdit->append(m_process->read(10000));
  }
  
  void PackmolDialog::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
  {
    ui.runButton->setEnabled(true);
    ui.abortButton->setEnabled(false);
  }


}

#include "packmoldialog.moc"
