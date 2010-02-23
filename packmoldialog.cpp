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
    : QDialog(parent, f)
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
    
  void PackmolDialog::solvUpdateSoluteNumber()
  {
  
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


}

#include "packmoldialog.moc"
