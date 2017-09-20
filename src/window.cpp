/*
Copyright 2007-2017 Jan Niklas Hasse <jhasse@gmail.com>
For conditions of distribution and use, see copyright notice in LICENSE.txt
*/

#include "window.hpp"
#include "jngl.hpp"

namespace jngl {
	double Window::GetTextWidth(const std::string& text) {
		return fonts_[fontSize_][fontName_]->getTextWidth(text);
	}

	int Window::getLineHeight() {
		return fonts_[fontSize_][fontName_]->getLineHeight();
	}

	void Window::setLineHeight(int h) {
		return fonts_[fontSize_][fontName_]->setLineHeight(h);
	}

	std::shared_ptr<FontImpl> Window::getFontImpl() {
		return fonts_[fontSize_][fontName_];
	}

	void Window::print(const std::string& text, const int xposition, const int yposition) {
		fonts_[fontSize_][fontName_]->print(xposition, yposition, text);
	}

	void Window::setFont(const std::string& filename) {
		if (fonts_[fontSize_].find(filename) == fonts_[fontSize_].end()) {
			auto font = std::make_shared<FontImpl>(filename, fontSize_);
			fonts_[fontSize_][filename] = font;
		}
		fontName_ = filename;
	}

	std::string Window::getFont() const {
		return fontName_;
	}

	void Window::setFontByName(const std::string& name) {
		Window::setFont(GetFontFileByName(name));
	}

	int Window::getFontSize() const {
		return fontSize_;
	}

	void Window::setFontSize(const int size) {
		const int oldSize = fontSize_;
		fontSize_ = size;
		try {
			setFont(fontName_); // We changed the size we also need to reload the current font
		} catch(std::exception& e) { // Something went wrong ...
			fontSize_ = oldSize; // ... so let's set fontSize_ back to the previous size
			throw e;
		}
	}

	bool Window::getMouseDown(mouse::Button button) {
		return mouseDown_.at(button);
	}

	bool Window::getMousePressed(mouse::Button button) {
		if (mousePressed_.at(button)) {
			needToBeSetFalse_.push(&mousePressed_[button]);
			return true;
		}
		return false;
	}

	void Window::setMousePressed(mouse::Button button, bool p) {
		mousePressed_.at(button) = p;
	}

	void Window::setMouseDown(mouse::Button button, bool d) {
		mouseDown_.at(button) = d;
	}

	bool Window::getFullscreen() const {
		return fullscreen_;
	}

	bool Window::getMouseVisible() const {
		return isMouseVisible_;
	}

	bool Window::getRelativeMouseMode() const {
		return relativeMouseMode;
	}

	int Window::getWidth() const {
		return width_;
	}

	int Window::getHeight() const {
		return height_;
	}

	bool Window::isMultisampleSupported() const {
		return isMultisampleSupported_;
	}

	bool Window::isRunning() {
		return running_;
	}

	void Window::quit() {
		running_ = false;
	}

	void Window::cancelQuit() {
		running_ = true;
	}

	bool Window::getKeyDown(key::KeyType key) {
		if (key == key::Any) {
			for (auto it = keyDown_.begin(); it != keyDown_.end(); ++it) {
				if (it->second) {
					return true;
				}
			}
			for (auto it = characterDown_.begin(); it != characterDown_.end(); ++it) {
				if (it->second) {
					return true;
				}
			}
			return false;
		}
		return keyDown_[GetKeyCode(key)];
	}

	bool Window::getKeyPressed(key::KeyType key) {
		if (key == key::Any) {
			if (anyKeyPressed_) {
				needToBeSetFalse_.push(&anyKeyPressed_);
				return getKeyDown(jngl::key::Any);
			}
		} else if(keyPressed_[GetKeyCode(key)]) {
			needToBeSetFalse_.push(&keyPressed_[GetKeyCode(key)]);
			return true;
		}
		return false;
	}

	void Window::setKeyPressed(const key::KeyType key, bool p) {
		keyPressed_[GetKeyCode(key)] = p;
	}

	void Window::setKeyPressed(const std::string &key, bool p) {
		characterPressed_[key] = p;
	}

	bool keyDown(const char key) {
		std::string temp; temp.append(1, key);
		return keyDown(temp);
	}

	bool keyPressed(const char key) {
		std::string temp; temp.append(1, key);
		return keyPressed(temp);
	}

	void Window::updateKeyStates() {
		while (!needToBeSetFalse_.empty()) {
			*(needToBeSetFalse_.top()) = false;
			needToBeSetFalse_.pop();
		}
		mouseWheel_ = 0;
	}

	double Window::getMouseWheel() const {
		return mouseWheel_;
	}

	void Window::mainLoop() {
		Finally _([&]() {
			currentWork_.reset((jngl::Work*)0);
		});
		while (running_) {
			stepIfNeeded();
			draw();
			jngl::swapBuffers();
		}
	}

	void Window::resetFrameLimiter() {
		oldTime = jngl::getTime();
		stepsPerFrame = 1;
	}

	unsigned int Window::getStepsPerSecond() const {
		return static_cast<unsigned int>(1.0 / timePerStep);
	}

	void Window::setStepsPerSecond(const unsigned int stepsPerSecond) {
		timePerStep = 1.0 / static_cast<double>(stepsPerSecond);
	}

	void Window::stepIfNeeded() {
		const auto dif = jngl::getTime() - oldTime - timePerStep * stepsPerFrame;
		if (dif > 1) { // something is wrong
			oldTime = jngl::getTime(); // ignore this frame
		} else if (dif > timePerStep) {
			stepsPerFrame += dif;
		}
		if (stepsPerFrame < 0.51f) {
			stepsPerFrame = 1.0f;
		}
		for (int i = 0; i < int(stepsPerFrame + 0.5); ++i) {
			oldTime += timePerStep;
			jngl::updateInput();
			if (currentWork_) {
				currentWork_->step();
			}
			for (auto& job : jobs) {
				job->step();
			}
			if (!jngl::running() && currentWork_) {
				currentWork_->onQuitEvent();
			}
			while (changeWork_) {
				changeWork_ = false;
				currentWork_ = newWork_;
				newWork_.reset((jngl::Work*)0);
				currentWork_->onLoad();
			}
		}
		auto timeToSleep = oldTime - jngl::getTime();
		if (timeToSleep > 0.01) {
			jngl::sleep(int(timeToSleep * 900));
			stepsPerFrame -= 0.1f;
		} else if (timeToSleep > 0) {
			oldTime = jngl::getTime();
		}
		if (timeToSleep > timePerStep && stepsPerFrame > 0.6) {
			stepsPerFrame -= float(timeToSleep - timePerStep);
		}
	}

	void Window::draw() const {
		if (currentWork_) {
			currentWork_->draw();
		} else {
			jngl::print("No work set. Use jngl::setWork", -50, -5);
		}
		for (auto& job : jobs) {
			job->draw();
		}
	}

	void Window::setWork(std::shared_ptr<Work> work) {
		if (!currentWork_) {
			debug("setting current work to "); debug(work.get()); debug("\n");
			currentWork_ = work;
		} else {
			debug("change work to "); debug(work.get()); debug("\n");
			changeWork_ = true;
			newWork_ = work;
		}
	}

	void Window::addJob(std::shared_ptr<Job> job) {
		jobs.push_back(job);
	}

	std::shared_ptr<Work> Window::getWork() {
		return currentWork_;
	}

	void Window::setConfigPath(const std::string& path) {
		configPath = path;
	}

	std::string Window::getConfigPath() const {
		return configPath;
	}

	bool Window::isMultitouch() const {
		return multitouch;
	}
}
